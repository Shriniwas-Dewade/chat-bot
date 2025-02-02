#include "bot.h"

bot::bot(const std::string& modelPath) : modelPath(modelPath)
{
    std::cout << "\033[1;34mLoading model: " << modelPath << "\033[0m\n";

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 7;
    this->model = llama_model_load_from_file(modelPath.c_str(), model_params);

    if (model == nullptr)
    {
        std::cerr << "\033[1;31mFailed to load model: " << modelPath << "\033[0m\n";
        std::exit(1);
    }

    this->contextParams = new llama_context_params();
    this->contextParams->n_ctx = 1024;
    this->contextParams->n_batch = 1024;
    this->contextParams->n_threads = 7;
    this->contextParams->n_threads_batch = this->contextParams->n_threads;

    std::cout << "Number of threads: " << this->contextParams->n_threads << std::endl;

    this->context = llama_init_from_model(this->model, *this->contextParams);

    if (context == nullptr)
    {
        std::cerr << "\033[1;31mFailed to create context!\033[0m\n";
        std::exit(1);
    }

    this->sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(this->sampler, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(this->sampler, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(this->sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    std::cout << "\033[1;34mModel loaded successfully!\033[0m\n";
}

bot::~bot()
{
    if (model != nullptr)
    {
        llama_model_free(model);
    }

    if (context != nullptr)
    {
        llama_free(context);
    }

    if (sampler != nullptr)
    {
        llama_sampler_free(sampler);
    }
}

void bot::startChat()
{
    std::string input;
    std::cout << "\033[1;32mChatbot Ready! Type 'exit' to quit.\033[0m\n";

    while (true)
    {
        printUser("You: ");
        std::getline(std::cin, input);

        if (input == "exit")
        {
            break;
        }

        std::string response = getResponse(input);
        printBot("Bot: " + response);
    }
}

std::string bot::getResponse(const std::string& input) 
{
    const llama_vocab *vocab = llama_model_get_vocab(model);
    if (!vocab) 
    {
        std::cerr << "Failed to get vocabulary from model!" << std::endl;
        return "Error: Could not tokenize input.";
    }

    bool is_first = llama_get_kv_cache_used_cells(context) == 0;
    int32_t num_tokens = -llama_tokenize(vocab, input.c_str(), input.size(), nullptr, 0, is_first, true);
    
    if (num_tokens < 0) 
    {
        std::cerr << "Tokenization failed!" << std::endl;
        return "Error: Tokenization failed.";
    }

    std::vector<llama_token> tokens(num_tokens);
    if (llama_tokenize(vocab, input.c_str(), input.size(), tokens.data(), tokens.size(), is_first, true) < 0) 
    {
        std::cerr << "Failed to tokenize input text!" << std::endl;
        return "Error: Could not process input.";
    }


    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());


    if (llama_decode(context, batch)) 
    {
        std::cerr << "Failed to run inference!" << std::endl;
        return "Error: Model failed to process input.";
    }

    std::string response;
    llama_token new_token_id;
    int max_tokens = 256;
    int token_count = 0;

    while (token_count < max_tokens)
    {
        new_token_id = llama_sampler_sample(sampler, context, -1);

        if (llama_vocab_is_eog(vocab, new_token_id))
        {
            std::cout << "\n[DEBUG] End of generation token received." << std::endl;
            break;
        }

        if (new_token_id < 0 || new_token_id >= llama_vocab_n_tokens(vocab))  
        {
            std::cerr << "[ERROR] Invalid token ID: " << new_token_id << std::endl;
            break;
        }

        char buf[256] = {0}; 
        int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);

        if (n < 0)  
        {
            std::cerr << "[ERROR] Failed to convert token to text!" << std::endl;
            break;
        }

        std::string piece(buf, n);

        for (char &c : piece)  
        {
            if (!isprint(c) && c != '\n' && c != ' ')  
            {
                c = ' ';
            }
        }

        response += piece;
        token_count++;
    }

    return response;
}


void bot::printUser(const std::string& text)
{
    std::cout << "\033[1;36m" << text << "\033[0m";
}

void bot::printBot(const std::string& text)
{
    std::cout << "\033[1;33m" << text << "\033[0m\n";
}
