#ifndef CRM_CLIENTES_H
#define CRM_CLIENTES_H

#include <string>
#include <vector>
#include "cliente.h"

// Módulo de Clientes com isolamento por negócio (multi-empresa).
// "donoId" é sempre o id do usuário (tipo CLIENTE) que está logado —
// obtenha esse valor com obterIdUsuarioPorUsername() logo após o login.

// Cadastra um cliente vinculado ao negócio de donoId.
// Em caso de erro (ex: CPF já cadastrado), retorna false e preenche mensagemErro.
bool cadastrarClienteNegocio(int donoId, const std::string &cpf, const std::string &nome,
                             const std::string &email, const std::string &telefone,
                             std::string &mensagemErro);

// Lista somente os clientes que pertencem ao negócio de donoId.
std::vector<Cliente> listarClientesDoNegocio(int donoId);

// Atualiza nome/email/telefone de um cliente, mas só se ele pertencer a donoId
// (evita um negócio editar cliente de outro).
bool atualizarClienteNegocio(int idCliente, const std::string &cpf, const std::string &nome,
                             const std::string &email, const std::string &telefone,
                             std::string &mensagemErro);

// Remove um cliente, mas só se ele pertencer a donoId.
bool deletarClienteDoNegocio(int donoId, int clienteId);

#endif
