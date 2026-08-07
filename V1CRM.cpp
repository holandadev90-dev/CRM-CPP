#include <iostream>
#include <string>
#include <vector>
using namespace std;
void cadastrarcliente();
void listagem();

int opcao;
int proximoId = 1;
class Cliente
{
private:
    // 1. primeiro encapsulado (ninguem de fora altera diretamente)
    int id;
    string nome;
    string email;
    string telefone;

public:
    // 2. contrutor: acionado quando criarmos um novo cliente
    Cliente(int id, string nome, string email, string telefone)
    {
        this->id = id;
        this->nome = nome;
        this->email = email;
        this->telefone = telefone;
    }
    // 3. getters: metodos de LER as informacoes de forma segura
    int getId() { return id; }
    string getNome() { return nome; }
    string getEmail() { return email; }
    string getTelefone() { return telefone; }

    // 4. Metodo utilitario para exibir os dados do cliente no terminal
    void exibirCliente()
    {
        cout << "ID: " << id;
        cout << "\nnome: " << nome;
        cout << "\ntelefone: " << telefone;
        cout << "\nEmail: " << email;
    }
};
vector<Cliente> clientes;
void listagem()
{
    cout << "Quantidade de clientes: " << clientes.size() << endl;

    for (Cliente cliente : clientes)
    {
        cliente.exibirCliente();

        cout << "\n==========================\n";
    }
}
void menu()
{
    while (opcao != 4)
    {
        cout << "=====--= MENU =--=====\n";
        cout << "========================\n";
        cout << "1 - Cadastrar cliente\n";
        cout << "2 - Listar clientes\n";
        cout << "3 - Vendas confirmadas\n";
        cout << "4 - Sair\n";
        cout << "==-=-=-=-=-=-=-=-=-=-=-=-==\n\n";
        cin >> opcao;

        if (opcao == 1)
        {
            cout << "Acesso a aba de cadastro de clientes\n";
            cout << "=======================================\n";
            cadastrarcliente();
        }
        else if (opcao == 2)
        {
            cout << "Acesso a aba de listagem de clientes\n";
            cout << "=======================================\n";
            listagem();
        }
        else if (opcao == 3)
        {
            cout << "Acesso a aba de vendas confirmadas\n";
            cout << "=======================================\n";
        }
        else if (opcao == 4)
        {
            cout << "Saindo do CRM...\n";
        }
    }
}
void cadastrarcliente()
{
    string nome, email, telefone;

    cout << "=====--= CADASTRAR CLIENTE =--=====\n";

    cout << "digite o nome do cliente:\n";
    cin.ignore();
    getline(cin, nome);
    cout << "digite um email para cadastro:\n";
    cin >> email;
    cout << "digite um telefone para cadastro:\n";
    cin >> telefone;
    cout << "======================================\n";

    // 2. você passa as variaveis capturadas para a classe!
    Cliente novoCliente(proximoId, nome, email, telefone);

    // guarda os clientes no vector
    clientes.push_back(novoCliente);

    proximoId++;

    // 3.exibe o resultado
    cout << "\nCliente cadastrado com sucesso!\n";
    cout << "========= DADOS DO CLIENTE ========\n";
    novoCliente.exibirCliente();
    cout << "\n=======------========-------=======\n";
    return menu();
}

int main()
{
    menu();
}