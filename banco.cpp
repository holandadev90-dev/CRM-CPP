#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "banco.h"
#include "cliente.h"
#include "sqlite3.h"
#include <cstring>

using namespace std;
sqlite3 *db = nullptr; // Aqui ela nasce de verdade

// --- Suporte a migracao de schema (multi-empresa) ---
// Verifica se uma coluna ja existe numa tabela (evita erro ao rodar ALTER TABLE de novo)

struct RegistroHistorico
{
    std::string cliente;
    std::string servico;
    float valor;
    std::string data;
};

static bool colunaExiste(sqlite3 *conexao, const string &tabela, const string &coluna)
{
    bool existe = false;
    string sql = "PRAGMA table_info(" + tabela + ");";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(conexao, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char *nomeCol = sqlite3_column_text(stmt, 1); // coluna 1 = "name"
            if (nomeCol && coluna == reinterpret_cast<const char *>(nomeCol))
            {
                existe = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
    }
    return existe;
}

// Garante que a tabela tenha a coluna usuario_id, adicionando se necessario
void garantirColunaUsuarioId(sqlite3 *conexao, const string &tabela)
{
    if (!colunaExiste(conexao, tabela, "usuario_id"))
    {
        string sqlAlter = "ALTER TABLE " + tabela + " ADD COLUMN usuario_id INTEGER REFERENCES usuarios(id);";
        char *erroMsg = nullptr;
        sqlite3_exec(conexao, sqlAlter.c_str(), NULL, 0, &erroMsg);
        if (erroMsg)
        {
            sqlite3_free(erroMsg);
        }
    }
}

bool autenticarUsuarioCompleto(std::string username, std::string senha, std::string &cargoRetorno, std::string &nomeExibicaoRetorno)
{
    if (!db)
    {
        return false; // Evita crash se o banco não estiver aberto
    }

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT tipo, nome_exibicao FROM usuarios WHERE username = ? AND senha = ?;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        if (stmt)
            sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, senha.c_str(), -1, SQLITE_TRANSIENT);

    bool logado = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *cargoDb = sqlite3_column_text(stmt, 0);
        if (cargoDb)
        {
            cargoRetorno = reinterpret_cast<const char *>(cargoDb);
        }
        else
        {
            cargoRetorno = "CLIENTE";
        }

        const unsigned char *nomeDb = sqlite3_column_text(stmt, 1);
        if (nomeDb)
        {
            nomeExibicaoRetorno = reinterpret_cast<const char *>(nomeDb);
        }
        else
        {
            nomeExibicaoRetorno = username;
        }

        logado = true;
    }

    // OBRIGATÓRIO: Sempre finalize o statement para evitar vazamento de memória do SQLite
    sqlite3_finalize(stmt);
    return logado;
}
string usuarioLogadoAtual = "";

string ObterDataeHora()
{
    time_t agora = time(0);
    tm *dataHora = localtime(&agora);

    stringstream ss;
    ss << setfill('0') << setw(2) << dataHora->tm_mday << "/"
       << setfill('0') << setw(2) << (dataHora->tm_mon + 1) << "/"
       << (dataHora->tm_year + 1900) << " "
       << setfill('0') << setw(2) << dataHora->tm_hour << ":"
       << setfill('0') << setw(2) << dataHora->tm_min;

    return ss.str();
}

bool realizarLogin()
{
    string usuario, senha;

    cout << "\n========================================\n";
    cout << "          TELA DE LOGIN - CRM           \n";
    cout << "========================================\n";
    cout << "Usuario: ";
    getline(cin, usuario);
    cout << "Senha: ";
    getline(cin, senha);
    cout << "----------------------------------------\n";

    string sql = "SELECT * FROM usuarios WHERE username = '" + usuario + "' AND senha = '" + senha + "';";
    sqlite3_stmt *stmt;

    bool loginSucesso = false;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            loginSucesso = true;
        }
        sqlite3_finalize(stmt);
    }

    if (loginSucesso)
    {
        usuarioLogadoAtual = usuario; // Salva quem está logado
        cout << "\n[SUCESSO] Login realizado com sucesso! Bem-vindo, " << usuario << ".\n";
        return true;
    }
    else
    {
        cout << "\n[ERRO] Usuario ou senha incorretos!\n";
        cout << "Pressione ENTER para tentar novamente...";
        cin.get();
        return false;
    }
}

bool cadastrarNovoUsuario(std::string usuario, std::string senha, std::string tipo, std::string nomeExibicao, std::string tipoNegocio, std::string cpf)
{
    sqlite3_stmt *stmt;
    std::string sql = "INSERT INTO usuarios (username, senha, tipo, nome_exibicao, tipo_negocio, cpf) VALUES (?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, usuario.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, senha.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, tipo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, nomeExibicao.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, tipoNegocio.c_str(), -1, SQLITE_TRANSIENT);

    if (cpf.empty())
    {
        sqlite3_bind_null(stmt, 6);
    }
    else
    {
        sqlite3_bind_text(stmt, 6, cpf.c_str(), -1, SQLITE_TRANSIENT);
    }

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return sucesso;
}

void inicializarBanco()
{
    int resultado = sqlite3_open("crm.db", &db);
    if (resultado != SQLITE_OK)
    {
        cout << "Erro ao abrir o banco de dados!\n";
        return;
    }

    // Ativa suporte a Chaves Estrangeiras no SQLite
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    // 1. TABELAS BASE DO SISTEMA

    // Tabela de Clientes
    string sqlClientes = "CREATE TABLE IF NOT EXISTS clientes ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "cpf TEXT NOT NULL UNIQUE, "
                         "nome TEXT NOT NULL, "
                         "email TEXT NOT NULL, "
                         "telefone TEXT NOT NULL);";

    // Tabela de Usuários (Donos de estabelecimentos / Administradores)
    string sqlUsuarios = "CREATE TABLE IF NOT EXISTS usuarios ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "username TEXT NOT NULL UNIQUE, "
                         "senha TEXT NOT NULL, "
                         "tipo TEXT NOT NULL DEFAULT 'CLIENTE', "
                         "nome_exibicao TEXT, "
                         "tipo_negocio TEXT DEFAULT 'Geral', "
                         "cpf TEXT UNIQUE);";

    // Tabela de Produtos e Serviços
    string sqlProdutos = "CREATE TABLE IF NOT EXISTS produtos_servicos ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "usuario_id INTEGER, "
                         "nome TEXT NOT NULL, "
                         "descricao TEXT, "
                         "tipo TEXT CHECK(tipo IN ('produto', 'servico')) NOT NULL, "
                         "sku TEXT UNIQUE, "
                         "preco_custo REAL NOT NULL, "
                         "preco_venda REAL NOT NULL, "
                         "estoque_atual INTEGER DEFAULT 0, "
                         "estoque_minimo INTEGER DEFAULT 0, "
                         "ativo INTEGER CHECK(ativo IN (0, 1)) DEFAULT 1);";

    // Tabela de Profissionais / Barbeiros
    string sqlBarbeiros = "CREATE TABLE IF NOT EXISTS barbeiros ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "usuario_id INTEGER, "
                          "nome TEXT NOT NULL, "
                          "foto_path TEXT, "
                          "ativo INTEGER DEFAULT 1, "
                          "status_atendimento TEXT DEFAULT 'Livre', "
                          "FOREIGN KEY(usuario_id) REFERENCES usuarios(id));";

    // Tabela de Agendamentos
    string sqlAgendamentos = "CREATE TABLE IF NOT EXISTS agendamentos ("
                             "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                             "usuario_id INTEGER, "
                             "barbeiro_id INTEGER, "
                             "cliente_nome TEXT NOT NULL, "
                             "servico TEXT NOT NULL, "
                             "valor REAL DEFAULT 0.0, "
                             "data_hora TEXT NOT NULL, "
                             "FOREIGN KEY(usuario_id) REFERENCES usuarios(id), "
                             "FOREIGN KEY(barbeiro_id) REFERENCES barbeiros(id));";

    // Tabela de Histórico de Faturamento
    string sqlHistorico = "CREATE TABLE IF NOT EXISTS faturamento_historico ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "usuario_id INTEGER, "
                          "barbeiro_id INTEGER, "
                          "cliente TEXT, "
                          "servico TEXT, "
                          "valor REAL, "
                          "data TEXT);";

    // Tabela de Configurações do Pix (Atualizada com a coluna caminho_qrcode)
    string sqlPix = "CREATE TABLE IF NOT EXISTS configuracoes_pix ("
                    "usuario_id INTEGER PRIMARY KEY, "
                    "tipo_chave TEXT NOT NULL, "
                    "chave_pix TEXT NOT NULL, "
                    "caminho_qrcode TEXT DEFAULT '', "
                    "FOREIGN KEY(usuario_id) REFERENCES usuarios(id) ON DELETE CASCADE);";

    // 2. ÍNDICES DE PERFORMANCE
    string sqlIndices =
        "CREATE INDEX IF NOT EXISTS idx_clientes_usuario ON clientes(usuario_id);"
        "CREATE INDEX IF NOT EXISTS idx_agendamentos_usuario ON agendamentos(usuario_id);"
        "CREATE INDEX IF NOT EXISTS idx_produtos_usuario ON produtos_servicos(usuario_id);"
        "CREATE INDEX IF NOT EXISTS idx_historico_usuario ON faturamento_historico(usuario_id);";

    char *erroMsg = nullptr;

    // 3. EXECUÇÃO DAS CRIAÇÕES NO BANCO
    sqlite3_exec(db, sqlClientes.c_str(), nullptr, nullptr, &erroMsg);
    sqlite3_exec(db, sqlUsuarios.c_str(), nullptr, nullptr, &erroMsg);
    sqlite3_exec(db, sqlProdutos.c_str(), nullptr, nullptr, &erroMsg);
    sqlite3_exec(db, sqlBarbeiros.c_str(), nullptr, nullptr, &erroMsg);
    sqlite3_exec(db, sqlAgendamentos.c_str(), nullptr, nullptr, &erroMsg);
    sqlite3_exec(db, sqlHistorico.c_str(), nullptr, nullptr, &erroMsg);
    sqlite3_exec(db, sqlPix.c_str(), nullptr, nullptr, &erroMsg);

    // Executa os Índices
    sqlite3_exec(db, sqlIndices.c_str(), nullptr, nullptr, &erroMsg);

    // 4. SEED DE ADMIN PADRÃO
    string sqlAdminCheck = "INSERT INTO usuarios (username, senha, tipo, nome_exibicao, tipo_negocio) "
                           "SELECT 'admin', '123', 'ADMIN', 'Dev - José', 'Sistema' "
                           "WHERE NOT EXISTS (SELECT 1 FROM usuarios WHERE username = 'admin');";

    sqlite3_exec(db, sqlAdminCheck.c_str(), nullptr, nullptr, &erroMsg);

    // 5. MIGRAÇÕES DE COMPATIBILIDADE
    garantirColunaUsuarioId(db, "clientes");
    garantirColunaUsuarioId(db, "produtos_servicos");
    garantirColunaUsuarioId(db, "barbeiros");
    garantirColunaUsuarioId(db, "agendamentos");
    garantirColunaUsuarioId(db, "faturamento_historico");

    garantirColunaStatusAtendimento(db);

    // MIGRAÇÃO SEGURA DO PIX: Garante que o campo de imagem do QR Code exista no arquivo crm.db antigo
    sqlite3_exec(db, "ALTER TABLE configuracoes_pix ADD COLUMN caminho_qrcode TEXT DEFAULT '';", nullptr, nullptr, nullptr);
    // MIGRAÇÃO SEGURA PARA FOTO DO BARBEIRO (Trata bancos antigos)
    sqlite3_exec(db, "ALTER TABLE barbeiros ADD COLUMN foto_path TEXT DEFAULT '';", nullptr, nullptr, nullptr);
}

void salvarConfiguracaoPix(sqlite3 *db, int usuarioId, const std::string &chavePix, const std::string &nomeTitular, const std::string &caminhoQrCode)
{
    const char *sql = "INSERT OR REPLACE INTO configuracoes_pix (usuario_id, chave_pix, tipo_chave, caminho_qrcode) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, usuarioId);
        sqlite3_bind_text(stmt, 2, chavePix.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, nomeTitular.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, caminhoQrCode.c_str(), -1, SQLITE_TRANSIENT);

        // O step precisa continuar aqui para EFETIVAR o salvamento!
        sqlite3_step(stmt);

        sqlite3_finalize(stmt);
    }
}

void carregarConfiguracaoPix(sqlite3 *db, int usuarioId, char *outChave, size_t sizeChave, char *outTitular, size_t sizeTitular, char *outCaminhoQr, size_t sizeCaminhoQr)
{
    if (outChave && sizeChave > 0)
        outChave[0] = '\0';
    if (outTitular && sizeTitular > 0)
        outTitular[0] = '\0';
    if (outCaminhoQr && sizeCaminhoQr > 0)
        outCaminhoQr[0] = '\0';

    const char *sql = "SELECT chave_pix, tipo_chave, caminho_qrcode FROM configuracoes_pix WHERE usuario_id = ?;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, usuarioId);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *chave = (const char *)sqlite3_column_text(stmt, 0);
            const char *titular = (const char *)sqlite3_column_text(stmt, 1);
            const char *caminhoQr = (const char *)sqlite3_column_text(stmt, 2);

            if (chave)
            {
                strncpy(outChave, chave, sizeChave - 1);
                outChave[sizeChave - 1] = '\0';
            }
            if (titular)
            {
                strncpy(outTitular, titular, sizeTitular - 1);
                outTitular[sizeTitular - 1] = '\0';
            }
            if (caminhoQr)
            {
                strncpy(outCaminhoQr, caminhoQr, sizeCaminhoQr - 1);
                outCaminhoQr[sizeCaminhoQr - 1] = '\0';
            }
        }
        sqlite3_finalize(stmt);
    }
}
// Função para garantir que a coluna existe em tabelas antigas
void garantirColunaStatusAtendimento(sqlite3 *db)
{
    char *erroMsg = nullptr;
    // O tipo TEXT já aceita qualquer string ("Livre", "Ocupado", "Atrasado"),
    // então a estrutura atual do banco suporta o novo status sem alterações complexas.
    std::string sqlCheck = "ALTER TABLE barbeiros ADD COLUMN status_atendimento TEXT DEFAULT 'Livre';";

    sqlite3_exec(db, sqlCheck.c_str(), NULL, 0, &erroMsg);
    if (erroMsg)
    {
        // Se a coluna já existir, o SQLite retorna erro, o que é esperado e seguro ignorar.
        sqlite3_free(erroMsg);
    }
}
// Retorna o id do usuario a partir do username (usar logo apos o login dar certo,
// ja que autenticarUsuarioCompleto nao retorna o id para nao quebrar quem ja usa ela)
void alternarStatusAtendimento(sqlite3 *db, int barbeiroId)
{
    string statusAtual = "Livre";
    string querySelect = "SELECT status_atendimento FROM barbeiros WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;

    // 1. Busca o status atual
    if (sqlite3_prepare_v2(db, querySelect.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, barbeiroId);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char *val = sqlite3_column_text(stmt, 0);
            if (val)
                statusAtual = reinterpret_cast<const char *>(val);
        }
        sqlite3_finalize(stmt);
    }

    // 2. Define o novo status invertido
    string novoStatus = (statusAtual == "Livre") ? "Em Atendimento" : "Livre";

    // 3. Atualiza no banco de dados usando a mesma variável 'stmt'
    string queryUpdate = "UPDATE barbeiros SET status_atendimento = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(db, queryUpdate.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, novoStatus.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, barbeiroId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

bool inserirProdutoBanco(int usuario_id, const char *nome, float custo, float venda, int estoque)
{
    sqlite3 *db;
    // Abra sua conexão com o banco aqui...
    // Exemplo de query protegida contra SQL Injection:
    std::string sql = "INSERT INTO produtos_servicos (usuario_id, nome, custo, venda, estoque) VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, usuario_id);
    sqlite3_bind_text(stmt, 2, nome, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, custo);
    sqlite3_bind_double(stmt, 4, venda);
    sqlite3_bind_int(stmt, 5, estoque);

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return sucesso;
}

int obterIdUsuarioPorUsername(const std::string &username)
{
    if (!db)
        return -1;

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT id FROM usuarios WHERE username = ?;";
    int id = -1;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            id = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return id;
}

// Retorna uma string formatada como "09:00 - Nome do Cliente (Serviço)"
std::string buscarProximoAgendamento(sqlite3 *db, int barbeiroId, const std::string &dataHoje)
{
    std::string proximoInfo = "Nenhum agendamento próximo";

    // Mantém exatamente a sua Query SQL original com tolerância de -10 min
    std::string sql = "SELECT data_hora, cliente_nome FROM agendamentos "
                      "WHERE barbeiro_id = ? AND data_hora >= datetime('now', 'localtime', '-10 minutes') "
                      "ORDER BY data_hora ASC LIMIT 1;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, barbeiroId);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char *dataHora = sqlite3_column_text(stmt, 0);
            const unsigned char *cliente = sqlite3_column_text(stmt, 1);

            std::string dh = dataHora ? reinterpret_cast<const char *>(dataHora) : "";
            std::string cli = cliente ? reinterpret_cast<const char *>(cliente) : "";

            // Extrai apenas a hora "HH:MM" do formato "YYYY-MM-DD HH:MM:SS"
            std::string horaMinuto = (dh.length() >= 16) ? dh.substr(11, 5) : dh;

            // Formatação final simplificada: APENAS "HH:MM - Nome"
            proximoInfo = horaMinuto + " - " + cli;
        }
        sqlite3_finalize(stmt);
    }

    return proximoInfo;
}

void cadastrarcliente()
{
    string cpf, nome, email, telefone;

    cout << "=====--= CADASTRAR CLIENTE =--=====\n";
    cout << "Digite o CPF do cliente:\n";
    getline(cin, cpf);

    cout << "Digite o nome do cliente:\n";
    getline(cin, nome);

    cout << "Digite um email para cadastro:\n";
    getline(cin, email);

    cout << "Digite um telefone para cadastro:\n";
    getline(cin, telefone);
    cout << "======================================\n";

    string sql = "INSERT INTO clientes (cpf, nome, email, telefone) VALUES ('" + cpf + "', '" + nome + "', '" + email + "', '" + telefone + "');";

    char *erroMsg = nullptr;
    int resultado = sqlite3_exec(db, sql.c_str(), NULL, 0, &erroMsg);

    if (resultado == SQLITE_OK)
    {
        cout << "\n[SUCESSO] Cliente cadastrado com CPF e salvo no SQLite!\n";
    }
    else
    {
        cout << "\n[ERRO] Falha ao salvar (CPF ja cadastrado ou dado invalido): " << erroMsg << endl;
        sqlite3_free(erroMsg);
    }

    cout << "\nPressione ENTER para voltar ao menu...";
    cin.get();
}

void listagem()
{
    string sql = "SELECT * FROM clientes;";
    sqlite3_stmt *stmt;

    cout << "Acesso a aba de listagem de clientes\n";
    cout << "=======================================\n";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        int quantidade = 0;

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            string cpf = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            string nome = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            string email = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            string telefone = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

            Cliente c(id, cpf, nome, email, telefone);
            c.exibirCliente();
            cout << "\n=======================================\n";
            quantidade++;
        }
        sqlite3_finalize(stmt);
        cout << "Total de clientes cadastrados no banco: " << quantidade << endl;
    }

    cout << "\nPressione ENTER para voltar ao menu...";
    cin.get();
}

void vendasconfirmadas()
{
    cout << "=====--= VENDAS CONFIRMADAS =--=====\n";
    cout << "======================================\n\n";
    cout << "ID\tNOME\t\tEMAIL\t\tTELEFONE\n";

    string sql = "SELECT * FROM clientes;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            cout << sqlite3_column_int(stmt, 0) << "\t"
                 << sqlite3_column_text(stmt, 1) << "\t"
                 << sqlite3_column_text(stmt, 2) << "\t"
                 << sqlite3_column_text(stmt, 3) << endl;
        }
        sqlite3_finalize(stmt);
    }
    cout << "===---===-- FIM DA LISTA DE CLIENTES ===---===--\n";

    cout << "\nPressione ENTER para voltar ao menu...";
    cin.get();
}

void menuDeletarCliente()
{
    cout << "\n===-- EXCLUIR CLIENTE --===\n";
    cout << "1. Deletar diretamente por ID\n";
    cout << "2. Buscar por Nome para escolher qual deletar\n";
    cout << "0. Voltar ao Menu Principal\n";
    cout << "Escolha uma opcao: ";

    int excluiropc;
    cin >> excluiropc;
    cin.ignore();

    if (excluiropc == 0)
        return;

    int idParaDeletar = -1;

    if (excluiropc == 1)
    {
        cout << "Digite o ID do cliente: ";
        cin >> idParaDeletar;
        cin.ignore();

        string sqlBusca = "SELECT id, nome, email FROM clientes WHERE id = " + to_string(idParaDeletar) + ";";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, sqlBusca.c_str(), -1, &stmt, NULL) == SQLITE_OK)
        {
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                string nome = (const char *)sqlite3_column_text(stmt, 1);
                string email = (const char *)sqlite3_column_text(stmt, 2);

                cout << "\n----------------------------------------\n";
                cout << "CLIENTE ENCONTRADO:\n";
                cout << "ID: " << idParaDeletar << " | Nome: " << nome << " | Email: " << email << "\n";
                cout << "----------------------------------------\n";
            }
            else
            {
                cout << "\n[AVISO] Nenhum cliente encontrado com o ID " << idParaDeletar << ".\n";
                cout << "\nPressione ENTER para continuar...";
                cin.get();
                sqlite3_finalize(stmt);
                return;
            }
            sqlite3_finalize(stmt);
        }
    }
    else if (excluiropc == 2)
    {
        string nomeBusca;
        cout << "Digite o nome (ou parte dele): ";
        getline(cin, nomeBusca);

        string sqlBusca = "SELECT id, nome, email FROM clientes WHERE nome LIKE '%" + nomeBusca + "%';";
        sqlite3_stmt *stmt;

        cout << "\n--- CLIENTES ENCONTRADOS ---\n";
        int encontrados = 0;

        if (sqlite3_prepare_v2(db, sqlBusca.c_str(), -1, &stmt, NULL) == SQLITE_OK)
        {
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                encontrados++;
                int id = sqlite3_column_int(stmt, 0);
                string nome = (const char *)sqlite3_column_text(stmt, 1);
                string email = (const char *)sqlite3_column_text(stmt, 2);

                cout << "ID: " << id << " | Nome: " << nome << " | Email: " << email << "\n";
            }
            sqlite3_finalize(stmt);
        }

        if (encontrados == 0)
        {
            cout << "[AVISO] Nenhum cliente encontrado com esse nome.\n";
            cout << "\nPressione ENTER para continuar...";
            cin.get();
            return;
        }

        cout << "\nDigite o ID do cliente que deseja EXCLUIR (ou 0 para cancelar): ";
        cin >> idParaDeletar;
        cin.ignore();

        if (idParaDeletar == 0)
        {
            cout << "\nOperacao cancelada.\n";
            cout << "\nPressione ENTER para continuar...";
            cin.get();
            return;
        }
    }

    if (idParaDeletar > 0)
    {
        char confirma;
        cout << "\nTem certeza que deseja EXCLUIR o cliente ID " << idParaDeletar << "? (S/N): ";
        cin >> confirma;
        cin.ignore();

        if (confirma == 'S' || confirma == 's')
        {
            string sqlDelete = "DELETE FROM clientes WHERE id = " + to_string(idParaDeletar) + ";";
            sqlite3_exec(db, sqlDelete.c_str(), NULL, 0, NULL);

            if (sqlite3_changes(db) > 0)
            {
                cout << "\n[SUCESSO] Cliente removido com sucesso!\n";
            }
            else
            {
                cout << "\n[ERRO] Nao foi possivel apagar o cliente.\n";
            }
        }
        else
        {
            cout << "\n[CANCELADO] Exclusao cancelada pelo usuario.\n";
        }
    }

    cout << "\nPressione ENTER para retornar ao menu principal...";
    cin.get();
}

void menu()
{
    int opcao;
    do
    {
        cout << "\n=================--= MENU CRM (" << usuarioLogadoAtual << ") =--=================\n";
        cout << "1 - Cadastrar Cliente\n";
        cout << "2 - Listar Clientes\n";
        cout << "3 - Vendas Confirmadas\n";

        // Se quem logou foi o admin, mostra a opção exclusiva de expandir o negócio
        if (usuarioLogadoAtual == "admin")
        {
            cout << "4 - Cadastrar Nova Empresa / Comercio (Exclusivo Admin)\n";
        }

        cout << "0 - Sair / Logout\n";
        cout << "========================================================\n";
        cout << "Escolha uma opcao: ";

        if (!(cin >> opcao))
        {
            cin.clear();
            cin.ignore(10000, '\n');
            opcao = -1;
        }
        cin.ignore(); // Limpa o buffer

        switch (opcao)
        {
        case 1:
            cadastrarcliente();
            break;
        case 2:
            listagem();
            break;
        case 3:
            vendasconfirmadas();
            break;
        case 0:
            cout << "\nSaindo do CRM...\n";
            break;
        default:
            cout << "\nOpcao invalida! Tente novamente.\n";
        }
    } while (opcao != 0);
}

std::vector<DadosClienteCRM> obterListaClientes()
{
    std::vector<DadosClienteCRM> lista;
    sqlite3_stmt *stmt;

    // Consulta para pegar todos os usuários que são clientes
    std::string sql = "SELECT id, username, tipo, nome_exibicao, tipo_negocio FROM usuarios WHERE tipo = 'CLIENTE';";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            DadosClienteCRM cliente;
            cliente.id = sqlite3_column_int(stmt, 0);

            const unsigned char *user = sqlite3_column_text(stmt, 1);
            cliente.username = user ? reinterpret_cast<const char *>(user) : "";

            const unsigned char *tp = sqlite3_column_text(stmt, 2);
            cliente.tipo = tp ? reinterpret_cast<const char *>(tp) : "";

            const unsigned char *nomeEx = sqlite3_column_text(stmt, 3);
            cliente.nomeExibicao = nomeEx ? reinterpret_cast<const char *>(nomeEx) : "Nao definido";

            const unsigned char *nicho = sqlite3_column_text(stmt, 4);
            cliente.tipoNegocio = nicho ? reinterpret_cast<const char *>(nicho) : "Geral";

            lista.push_back(cliente);
        }
    }
    sqlite3_finalize(stmt);
    return lista;
}

std::string obterTipoNegocioPorUsername(const std::string &username)
{
    if (!db)
        return "Geral";

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT tipo_negocio FROM usuarios WHERE username = ?;";
    std::string tipoNegocio = "Geral";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char *val = sqlite3_column_text(stmt, 0);
            if (val)
            {
                tipoNegocio = reinterpret_cast<const char *>(val);
            }
        }
        sqlite3_finalize(stmt);
    }
    return tipoNegocio;
}