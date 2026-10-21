#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include "file_processor.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using namespace file_processor;

class FileProcessorServiceImpl final : public FileProcessorService::Service {
private:
    // ==========================================================================
    // IMPLEMENTAÇÃO SUA (MÉTODOS AUXILIARES)
    // ==========================================================================
    std::mutex log_mutex_;

    void LogOperation(const std::string& type, const std::string& service,
                      const std::string& file, const std::string& msg) {
        std::lock_guard<std::mutex> lock(log_mutex_);

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::string ts = std::ctime(&t);
        if (!ts.empty() && ts.back() == '\n') ts.pop_back();

        std::ofstream log("server.log", std::ios::app);
        log << "[" << ts << "] [" << type << "] servico=" << service
            << " arquivo=" << file << " msg=" << msg << std::endl;
    }

    // ==========================================================================
    // MÉTODOS AUXILIARES FORNECIDOS (NÃO ALTERAR)
    // ==========================================================================
    bool ReceiveFile(ServerReaderWriter<FileResponse, FileRequest>* stream,
                     std::string& filepath, std::string& original_name,
                     FileRequest& last_req) {
        FileRequest req;
        std::ofstream outfile;
        bool first = true;

        while (stream->Read(&req)) {
            if (first) {
                original_name = req.file_name();
                filepath = "/tmp/input_" + original_name;
                outfile.open(filepath, std::ios::binary);
                last_req = req;
                first = false;
            }
            if (req.has_file_content()) {
                outfile.write(req.file_content().content().c_str(),
                              req.file_content().content().size());
            }
        }
        if (outfile.is_open()) outfile.close();
        return !first;
    }

    Status SendFile(ServerReaderWriter<FileResponse, FileRequest>* stream,
                    const std::string& out_filepath, const std::string& out_filename) {
        std::ifstream infile(out_filepath, std::ios::binary);
        if (!infile) return Status(grpc::StatusCode::INTERNAL, "Falha ao ler output");

        FileResponse resp;
        resp.set_success(true);
        resp.set_file_name(out_filename);
        resp.set_status_message("Sucesso");

        char buffer[1024];
        while (infile.read(buffer, sizeof(buffer)) || infile.gcount() > 0) {
            FileChunk* chunk = new FileChunk();
            chunk->set_content(buffer, infile.gcount());
            resp.set_allocated_file_content(chunk);
            stream->Write(resp);
            resp.clear_file_content();
        }
        return Status::OK;
    }

public:
    // ==========================================================================
    // IMPLEMENTAÇÃO SUA (SERVIÇOS gRPC)
    // ==========================================================================
    Status CompressPDF(ServerContext* context,
                       ServerReaderWriter<FileResponse, FileRequest>* stream) override {
        std::string in_path, name;
        FileRequest first_req;

        // 1. Recebe o arquivo do cliente
        if (!ReceiveFile(stream, in_path, name, first_req)) {
            LogOperation("ERRO", "CompressPDF", "-", "Nenhum dado recebido do cliente");
            return Status(grpc::StatusCode::INVALID_ARGUMENT, "Nenhum arquivo recebido");
        }

        // 2. Monta o comando do Ghostscript
        std::string out_path = "/tmp/output_" + name;
        std::string cmd =
            "gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/ebook "
            "-dNOPAUSE -dQUIET -dBATCH -sOutputFile=\"" + out_path + "\" \"" +
            in_path + "\" 2>/dev/null";

        // 3. Executa
        int rc = std::system(cmd.c_str());

        // 4. Em caso de falha: loga, limpa e devolve erro
        if (rc != 0) {
            LogOperation("ERRO", "CompressPDF", name,
                         "Ghostscript falhou, codigo de retorno " + std::to_string(rc));
            std::remove(in_path.c_str());
            std::remove(out_path.c_str());
            return Status(grpc::StatusCode::INTERNAL, "Falha ao comprimir o PDF");
        }

        // 5. Sucesso: loga e devolve o arquivo
        LogOperation("INFO", "CompressPDF", name, "Sucesso");
        Status result = SendFile(stream, out_path, name);

        // 6. Limpa temporários
        std::remove(in_path.c_str());
        std::remove(out_path.c_str());
        return result;
    }

    // ==========================================================================
    // MÉTODOS IGNORADOS PARA ESTA ENTREGA
    // ==========================================================================
    Status ConvertToTXT(ServerContext* /*context*/,
                        ServerReaderWriter<FileResponse, FileRequest>* /*stream*/) override {
        // Não é necessário implementar para esta atividade.
        return Status::OK;
    }

    Status ConvertImageFormat(ServerContext* /*context*/,
                              ServerReaderWriter<FileResponse, FileRequest>* /*stream*/) override {
        // Não é necessário implementar para esta atividade.
        return Status::OK;
    }

    Status ResizeImage(ServerContext* /*context*/,
                       ServerReaderWriter<FileResponse, FileRequest>* /*stream*/) override {
        // Não é necessário implementar para esta atividade.
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    FileProcessorServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Servidor gRPC operacional em " << server_address << std::endl;
    server->Wait();
}

int main() {
    RunServer();
    return 0;
}