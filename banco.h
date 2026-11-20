#ifndef BANCO_H
#define BANCO_H

#include <string>
#include <vector>
#include "sqlite3.h"

// ============================================================================
// CONEXÃO GLOBAL E ESTRUTURAS DE DADOS
// ============================================================================

// Conexão única com o banco de dados (definida em banco.cpp)
extern sqlite3 *db;

// Estrutura para modular os dados do usuário/cliente na listagem do CRM
struct DadosClienteCRM
{
    int id;
    std::string username;
    std::string tipo;
    std::string nomeExibicao;
    std::string tipoNegocio;
};

// ============================================================================
// CORE / INICIALIZAÇÃO E UTILITÁRIOS
// ============================================================================

void inicializarBanco();
std::string ObterDataeHora();

// Funções de manutenção/migração da estrutura do banco
void garantirColunaStatusAtendimento(sqlite3 *db);
void garantirColunaUsuarioId(sqlite3 *db, const std::string &tabela);

// ============================================================================
// MÓDULO: AUTENTICAÇÃO E USUÁRIOS (SISTEMA MULTI-USUÁRIO)
// ============================================================================

bool autenticarUsuarioCompleto(std::string username, std::string senha, std::string &cargoRetorno, std::string &nomeExibicaoRetorno);
bool cadastrarNovoUsuario(std::string usuario, std::string senha, std::string tipo, std::string nomeExibicao, std::string tipoNegocio, std::string cpf);
int obterIdUsuarioPorUsername(const std::string &username);
std::string obterTipoNegocioPorUsername(const std::string &username);

// ============================================================================
// MÓDULO: CONFIGURAÇÕES E PREFERÊNCIAS (PIX / ESTABELECIMENTO)
// ============================================================================

void salvarConfiguracaoPix(sqlite3 *db, int usuarioId, const std::string &chavePix, const std::string &nomeTitular, const std::string &caminhoQrCode);
void carregarConfiguracaoPix(sqlite3 *db, int usuarioId, char *outChave, size_t sizeChave, char *outTitular, size_t sizeTitular, char *outCaminhoQr, size_t sizeCaminhoQr);

// ============================================================================
// MÓDULO: CLIENTES E ATENDIMENTO
// ============================================================================

void cadastrarcliente();
void menuDeletarCliente();
std::vector<DadosClienteCRM> obterListaClientes();

// ============================================================================
// MÓDULO: BARBEARIA / AGENDAMENTOS / CADEIRAS
// ============================================================================

void alternarStatusAtendimento(sqlite3 *db, int barbeiroId);
std::string buscarProximoAgendamento(sqlite3 *db, int barbeiroId, const std::string &dataHoje);

// ============================================================================
// MÓDULO: PRODUTOS, ESTOQUE E VENDAS
// ============================================================================

bool inserirProdutoBanco(int usuario_id, const char *nome, float custo, float venda, int estoque);
void vendasconfirmadas();

// ============================================================================
// INTERFACE DE CONSOLE / MENUS LEGADOS
// ============================================================================

bool realizarLogin();
void listagem();
void menu();

#endif // BANCO_H