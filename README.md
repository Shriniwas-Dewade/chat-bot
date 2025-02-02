# Chatbot using llama.cpp

## **Project Overview**
This is a C++-based chatbot utilizing `llama.cpp` to perform natural language processing (NLP) on local models such as Mistral-7B or Llama 2. The chatbot can process user inputs and generate responses efficiently using CPU or GPU acceleration.

---

## **Installation & Setup**

### **1. Install Dependencies**
Before building the project, ensure you have the necessary dependencies installed:

#### **For Ubuntu/Debian**:
```sh
sudo apt update && sudo apt install -y cmake g++ make git libopenblas-dev
```

#### **For Arch Linux**:
```sh
sudo pacman -Syu cmake gcc make git openblas
```

#### **For Fedora**:
```sh
sudo dnf install cmake gcc-c++ make git openblas-devel
```

---

### **2. Clone the Repository**
```sh
git clone https://github.com/yourusername/chat-bot.git
cd chat-bot
```

---

### **3. Download the Model (Mistral-7B or Llama-2-7B)**
You need a `.gguf` model file to run the chatbot. Download a model from Hugging Face:
```sh
wget --header="Authorization: Bearer <your_huggingface_token>" \
    https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF/resolve/main/mistral-7b-instruct-v0.2-q4_k_m.gguf
```
Move the model to the project folder:
```sh
mv mistral-7b-instruct-v0.2-q4_k_m.gguf chat-bot/
```

---

### **4. Build the Project**
```sh
mkdir -p build && cd build
cmake .. -DLLAMA_CUDA=ON  # Enable CUDA if using GPU
make -j$(nproc)
```

---

## **Usage**
Run the chatbot with the specified model:
```sh
./chatbot -m ../mistral-7b-instruct-v0.2-q4_k_m.gguf -c 4096 -t 8 --n-gpu-layers 50
```

**Command-line arguments:**
- `-m` → Path to the model file.
- `-c` → Context size (default: `4096`).
- `-t` → Number of CPU threads (default: `8`).
- `--n-gpu-layers` → Number of layers offloaded to GPU (adjust based on VRAM).

Example:
```sh
./chatbot -m ../mistral-7b-instruct-v0.2-q4_k_m.gguf -c 2048 -t 6 --n-gpu-layers 35
```

---

## **Running the Chatbot as an API**
You can run the chatbot as an OpenAI-compatible API:
```sh
./server -m ../mistral-7b-instruct-v0.2-q4_k_m.gguf --n-gpu-layers 50
```
Then, test with `curl`:
```sh
curl http://localhost:8080/v1/chat/completions \
    -H "Content-Type: application/json" \
    -d '{"model": "mistral-7b-instruct-v0.2-q4_k_m", "messages": [{"role": "user", "content": "Hello!"}]}'
```

---

## **Debugging & Performance Optimization**
- **Monitor VRAM usage:**
  ```sh
  watch -n 1 nvidia-smi
  ```
- **Reduce `--n-gpu-layers` if out of memory (OOM) errors occur.**
- **Run with debugging enabled:**
  ```sh
  export GGML_DEBUG=1
  ./chatbot
  ```
- **Use GDB to debug segmentation faults:**
  ```sh
  gdb ./chatbot
  run
  bt  # Get backtrace
  ```

---

## **Contributing**
Feel free to open issues or submit pull requests for improvements and bug fixes.

---