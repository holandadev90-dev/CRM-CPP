#ifndef CRM_PRODUTOS_H
#define CRM_PRODUTOS_H

#include <string>
#include <vector>
#include "sqlite3.h"

extern void RenderizarModalExcluirEstoque(sqlite3 *db, int usuario_id, int &idParaExcluir);

// Representa uma linha da tabela produtos_servicos.
// Struct simples (nao classe) de proposito: o modulo ImGui edita os campos
// direto nos widgets, e struct facilita isso sem precisar de getters/setters.
struct ProdutoServico
{
    int id = 0;
    std::string nome;
    std::string descricao;
    std::string tipo; // "produto" ou "servico"
    std::string sku;
    double precoCusto = 0.0;
    double precoVenda = 0.0;
    int estoqueAtual = 0;
    int estoqueMinimo = 0;
    bool ativo = true;
};

// Mesma logica de isolamento por negocio dos clientes: donoId = id do usuario logado.

bool cadastrarProdutoNegocio(int donoId, const ProdutoServico &produto, std::string &mensagemErro);

std::vector<ProdutoServico> listarProdutosDoNegocio(int donoId);

// Ajusta o estoque_atual (uso tipico: registrar entrada/saida apos uma venda)
bool atualizarEstoqueProduto(int donoId, int produtoId, int novoEstoque);

bool atualizarProdutoDoNegocio(int donoId, const ProdutoServico &produto);

bool deletarProdutoDoNegocio(int donoId, int produtoId);

std::vector<ProdutoServico> listarTodosProdutosGeral(sqlite3 *dbConn = nullptr);

void RenderizarJanelaProdutos(bool *p_open, int donoId, std::string nicho);

#endif
