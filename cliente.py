# cliente.py
import grpc
import notas_pb2
import notas_pb2_grpc


def run():
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = notas_pb2_grpc.GerenciadorNotasStub(channel)

        # Adicionar notas
        for disciplina, nota in [("CIC0001", 9.5), ("CIC0002", 7.0), ("CIC0003", 8.0)]:
            response = stub.AdicionarNota(
                notas_pb2.AdicionaNotaRequest(
                    ra="123",
                    cod_disciplina=disciplina,
                    ano=2025,
                    semestre=2,
                    nota=nota,
                )
            )
            print(f"Resposta do servidor: {response.msg}")

        # Consultar uma nota
        consulta = stub.ConsultarNota(
            notas_pb2.AlunoDisciplinaRequest(ra="123", cod_disciplina="CIC0001")
        )
        if consulta.sucesso:
            print(f"Nota consultada: {consulta.nota.nota}")
        else:
            print(f"Erro ao consultar: {consulta.msg_erro}")

        # Alterar uma nota
        alteracao = stub.AlterarNota(
            notas_pb2.AdicionaNotaRequest(
                ra="123", cod_disciplina="CIC0002", ano=2025, semestre=2, nota=8.5
            )
        )
        print(f"Alterar nota: {alteracao.msg}")

        # Calcular média
        media_response = stub.CalcularMedia(notas_pb2.AlunoRequest(ra="123"))
        if media_response.sucesso:
            print(f"Média do aluno 123: {media_response.media:.2f}")
        else:
            print(f"Erro ao calcular média: {media_response.msg_erro}")

        # --- DESAFIO (STREAMING) ---
        print("\n--- Listando todas as notas do aluno 123 (DESAFIO) ---")
        try:
            notas_stream = stub.ListarNotasAluno(notas_pb2.AlunoRequest(ra="123"))
            for nota in notas_stream:
                print(f" - Disciplina: {nota.cod_disciplina}, Nota: {nota.nota}")
        except grpc.RpcError as e:
            print(f"Erro ao chamar ListarNotasAluno: {e.details()}")


if __name__ == '__main__':
    run()