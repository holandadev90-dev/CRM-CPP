#include "cliente.h"
#include <iostream>

using namespace std;

// Atualizado para receber e atribuir os 5 parâmetros
Cliente::Cliente(int id, string cpf, string nome, string email, string telefone)
{
    this->id = id;
    this->cpf = cpf;
    this->nome = nome;
    this->email = email;
    this->telefone = telefone;
}

int Cliente::getId() { return id; }
string Cliente::getCpf() { return cpf; }
string Cliente::getNome() { return nome; }
string Cliente::getEmail() { return email; }
string Cliente::getTelefone() { return telefone; }

void Cliente::exibirCliente()
{
    cout << "ID: " << id;
    cout << "\nCPF: " << cpf;
    cout << "\nNome: " << nome;
    cout << "\nTelefone: " << telefone;
    cout << "\nEmail: " << email;
}