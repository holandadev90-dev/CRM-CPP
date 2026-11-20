#ifndef CLIENTE_H
#define CLIENTE_H

#include <string>

using namespace std;

class Cliente
{
private:
    int id;
    string cpf;
    string nome;
    string email;
    string telefone;

public:
    // Atualizado para receber 5 argumentos (incluindo o CPF)
    Cliente(int id, string cpf, string nome, string email, string telefone);

    int getId();
    string getCpf();
    string getNome();
    string getEmail();
    string getTelefone();

    void exibirCliente();
};

#endif