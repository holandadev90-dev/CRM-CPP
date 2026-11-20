#include "crm_clientes.h"
#include "banco.h"

bool cadastrarClienteNegocio(int donoId, const std::string &cpf, const std::string &nome,
                             const std::string &email, const std::string &telefone,
                             std::string &mensagemErro)
{
    if (!db)
    {
        mensagemErro = "Banco de dados nao inicializado.";
        return false;
    }

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "INSERT INTO clientes (cpf, nome, email, telefone, usuario_id) "
                      "VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        mensagemErro = "Erro ao preparar cadastro.";
        return false;
    }

    sqlite3_bind_text(stmt, 1, cpf.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, nome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, telefone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, donoId);

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!sucesso)
    {
        // Causa mais comum: CPF ja cadastrado (coluna e UNIQUE)
        mensagemErro = "CPF ja cadastrado ou dado invalido.";
    }
    sqlite3_finalize(stmt);
    return sucesso;
}

std::vector<Cliente> listarClientesDoNegocio(int donoId)
{
    std::vector<Cliente> lista;
    if (!db)
        return lista;

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT id, cpf, nome, email, telefone FROM clientes "
                      "WHERE usuario_id = ? ORDER BY nome;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, donoId);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);

            const unsigned char *cpfDb = sqlite3_column_text(stmt, 1);
            const unsigned char *nomeDb = sqlite3_column_text(stmt, 2);
            const unsigned char *emailDb = sqlite3_column_text(stmt, 3);
            const unsigned char *telDb = sqlite3_column_text(stmt, 4);

            std::string cpf = cpfDb ? reinterpret_cast<const char *>(cpfDb) : "";
            std::string nome = nomeDb ? reinterpret_cast<const char *>(nomeDb) : "";
            std::string email = emailDb ? reinterpret_cast<const char *>(emailDb) : "";
            std::string telefone = telDb ? reinterpret_cast<const char *>(telDb) : "";

            lista.push_back(Cliente(id, cpf, nome, email, telefone));
        }
        sqlite3_finalize(stmt);
    }
    return lista;
}

bool atualizarClienteNegocio(int idCliente, const std::string &cpf, const std::string &nome, const std::string &email, const std::string &telefone, std::string &mensagemErro)
{
    if (!db)
    {
        mensagemErro = "Banco de dados nao inicializado.";
        return false;
    }

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "UPDATE clientes SET cpf = ?, nome = ?, email = ?, telefone = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        mensagemErro = "Erro ao preparar atualizacao.";
        return false;
    }

    sqlite3_bind_text(stmt, 1, cpf.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, nome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, telefone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, idCliente);

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!sucesso)
        mensagemErro = "Erro ao atualizar dados do cliente.";

    sqlite3_finalize(stmt);
    return sucesso;
}

bool deletarClienteDoNegocio(int donoId, int clienteId)
{
    if (!db)
        return false;

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "DELETE FROM clientes WHERE id = ? AND usuario_id = ?;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, clienteId);
    sqlite3_bind_int(stmt, 2, donoId);

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    sqlite3_finalize(stmt);
    return sucesso;
}
