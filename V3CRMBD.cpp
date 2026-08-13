#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include "sqlite3.h" // Importa o motor do SQLite local

using namespace std;

// ==== PROTOTIPOS === //
////////////////////////
void deletarClientePornome();
void menuDeletarCliente();

// Ponteiro global para gerenciar a conexão com o banco de dados
sqlite3 *db = nullptr;

int excluiropc;

class Cliente
{
private:
    int id;
    string nome;
    string email;
    string telefone;

public:
    Cliente(int id, string nome, string email, string telefone)
    {
        this->id = id;
        this->nome = nome;
        this->email = email;
        this->telefone = telefone;
    }

    int getId() { return id; }
    string getNome() { return nome; }
    string getEmail() { return email; }
    string getTelefone() { return telefone; }

    void exibirCliente()
    {
        cout << "ID: " << id;
        cout << "\nNome: " << nome;
        cout << "\nTelefone: " << telefone;
        cout << "\nEmail: " << email;
    }
};

// --- FUNÇÃO DE DATA E HORA ---
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

// --- BANCO DE DADOS: INICIALIZAÇÃO ---
void inicializarBanco()
{
    // Abre (ou cria) o arquivo do banco de dados chamado "crm.db"
    int resultado = sqlite3_open("crm.db", &db);
    if (resultado != SQLITE_OK)
    {
        cout << "Erro ao abrir o banco de dados!\n";
        return;
    }

    // Comando SQL para criar a tabela se ela ainda não existir
    string sql = "CREATE TABLE IF NOT EXISTS clientes ("
                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "nome TEXT NOT NULL, "
                 "email TEXT NOT NULL, "
                 "telefone TEXT NOT NULL);";

    char *erroMsg = nullptr;
    sqlite3_exec(db, sql.c_str(), NULL, 0, &erroMsg);
}

// --- BANCO DE DADOS: CADASTRAR CLIENTE (INSERT) ---
void cadastrarcliente()
{
    string nome, email, telefone;

    cout << "=====--= CADASTRAR CLIENTE =--=====\n";
    cout << "Digite o nome do cliente:\n";
    cin.ignore();
    getline(cin, nome);
    cout << "Digite um email para cadastro:\n";
    getline(cin, email);
    cout << "Digite um telefone para cadastro:\n";
    getline(cin, telefone);
    cout << "======================================\n";

    // Prepara o comando SQL de inserção
    string sql = "INSERT INTO clientes (nome, email, telefone) VALUES ('" + nome + "', '" + email + "', '" + telefone + "');";

    char *erroMsg = nullptr;
    int resultado = sqlite3_exec(db, sql.c_str(), NULL, 0, &erroMsg);

    if (resultado == SQLITE_OK)
    {
        cout << "\nCliente cadastrado e salvo no SQLite com sucesso!\n";
    }
    else
    {
        cout << "\nErro ao salvar no banco: " << erroMsg << endl;
    }
}

// --- BANCO DE DADOS: LISTAR CLIENTES (SELECT) ---
void listagem()
{
    string sql = "SELECT * FROM clientes;";
    sqlite3_stmt *stmt;

    cout << "Acesso a aba de listagem de clientes\n";
    cout << "=======================================\n";

    // Prepara a consulta SQL
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        int quantidade = 0;

        // Percorre cada linha que o SQL retornou
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            string nome = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            string email = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            string telefone = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

            Cliente c(id, nome, email, telefone);
            c.exibirCliente();
            cout << "\n=======================================\n";
            quantidade++;
        }
        sqlite3_finalize(stmt);
        cout << "Total de clientes cadastrados no banco: " << quantidade << endl;
    }
}

void vendasconfirmadas()
{
    cout << "=====--= VENDAS CONFIRMADAS =--=====\n";
    cout << "======================================\n\n";
    cout << "ID\tNOME\tEMAIL\tTELEFONE\n";

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
}

void deletarClientePorNome(string nome)
{
    string sql = "DELETE FROM clientes WHERE nome LIKE '%" + nome + "%';";

    char *erroMsg = nullptr;
    int resultado = sqlite3_exec(db, sql.c_str(), NULL, 0, &erroMsg);

    if (resultado == SQLITE_OK)
    {
        int apagados = sqlite3_changes(db);
        if (apagados > 0)
        {
            cout << "\n[SUCESSO] " << apagados << " cliente(s) deletado(s) com sucesso!\n";
        }
        else
        {
            cout << "\n[AVISO] Nenhum cliente encontrado com esse nome.\n";
        }
    }
    else
    {
        cout << "\n[ERRO] Falha ao deletar: " << erroMsg << "\n";
        sqlite3_free(erroMsg);
    }
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

    // ==========================================
    // OPÇÃO 1: DELETAR POR ID (COM CONFIRMAÇÃO)
    // ==========================================
    if (excluiropc == 1)
    {
        cout << "Digite o ID do cliente: ";
        cin >> idParaDeletar;
        cin.ignore();

        // Busca os dados do cliente pelo ID no banco antes de apagar
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
    // ==========================================
    // OPÇÃO 2: BUSCAR POR NOME
    // ==========================================
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

    // ==========================================
    // ETAPA FINAL: CONFIRMAÇÃO E EXCLUSÃO
    // ==========================================
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
    int opcao = 0;

    while (opcao != 5)
    {
        cout << "\n\n=--===========---==== MENU =--===========---====\n";
        cout << "Data/Hora: " << ObterDataeHora() << endl;
        cout << "================================================\n\n";
        cout << "1 - === CADASTRAR CLIENTE ===\n\n";
        cout << "2 - === LISTAR CLIENTES ===\n\n";
        cout << "3 - === VENDAS CONFIRMADAS ===\n\n";
        cout << "4 - === EXCLUIR CLIENTE ===\n\n";
        cout << "5 - === SAIR ===\n\n";
        cout << "==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n";

        cin >> opcao;
        cin.ignore(); // Limpa o buffer para nao dar bugs nas proximas leituras de texto!

        if (opcao == 1)
            cadastrarcliente();
        else if (opcao == 2)
            listagem();
        else if (opcao == 3)
            vendasconfirmadas();
        else if (opcao == 4)
            menuDeletarCliente(); // Chama o menu de deletar e DEPOIS volta pro menu principal
        else if (opcao == 5)
            cout << "Saindo do CRM...\n";
        else
            cout << "Opcao invalida! Tente novamente.\n";
    }
}

int main()
{
    inicializarBanco(); // Conecta e cria o banco crm.db
    menu();
    sqlite3_close(db); // Fecha a conexão com o banco ao fechar o programa
    return 0;
}