#pragma once

#include <string>
#include "imgui.h"
#include "sqlite3.h"
#include <GL/gl.h>
#include "modulos.h"

enum TemaCRM
{
    TEMA_DARK_BARBER = 0,
    TEMA_CLASSIC_WOOD,
    TEMA_NEON_CYBER,
    TEMA_LIGHT_MINIMAL
};

// Variáveis globais de fundos e temas (armazenadas na memória em telas.cpp)
extern GLuint texturasFundos[4];
extern int temaFundoAtual;

// ============================================================================
// CORE / GERENCIAMENTO GLOBAL DE TELAS E JANELAS E TEMAS
// ============================================================================

void AplicarTema(TemaCRM tema);
void RenderizarTituloModal(const char *texto);
bool BotaoSucesso(const char *label, ImVec2 tamanho = ImVec2(0, 0));
bool BotaoCancelar(const char *label, ImVec2 tamanho = ImVec2(0, 0));
void CarregarFundosTemas();
void FecharTodasAsJanelasExclusivas();

// ============================================================================
// MÓDULO: GERENCIAMENTO E PAINÉIS ADMINISTRATIVOS
// ============================================================================

void RenderizarJanelaGerenciamento(bool *p_open);
void RenderizarJanelaAvancada(bool *p_open);
void RenderizarModalAdicionarFoto(sqlite3 *db);

// ============================================================================
// MÓDULO: ATENDIMENTO, BARBEARIA E CADEIRAS
// ============================================================================

void CadeirasBarber();
void CarregarHistoricoCadeira(sqlite3 *db, int cadeiraID, int dias = 30);
bool ChecarAgendamentoAtrasado(sqlite3 *db, int barbeiroID);
void RenderizarModalAlertaAtraso();

// ============================================================================
// MÓDULO: PRODUTOS, ESTOQUE E SERVIÇOS
// ============================================================================

void desenharJanelaProdutos(bool *p_open, void *db, int usuario_id);
void RenderizarJanelaProdutos(bool *p_open, int usuario_id, std::string tipoNegocio);

// ============================================================================
// MÓDULO: FINANCEIRO E PAGAMENTOS (PIX)
// ============================================================================

bool SelecionarImagemQrCode(char *outCaminho, size_t maxTam);
void RenderizarModalConfigPix(sqlite3 *db);
void RenderizarModalReceberPix();
void RenderizarModalFechamento(sqlite3 *db);
void InicializarDadosPix(sqlite3 *db);
GLuint CarregarTexturaImagem(const char *filename, int *out_width, int *out_height);
void CopiarImagemParaClipboard(const char *caminhoArquivo);

// ============================================================================
// NOVOS MÓDULOS FUTUROS (Reservado para Expansão)
// Exemplo: Petshop, Salão, Relatórios, Configurações
// ============================================================================