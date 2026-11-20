#ifndef SISTEMA_FINANCEIRO_H
#define SISTEMA_FINANCEIRO_H

#include <string>
#include <vector>

struct AgendamentoConcluido
{
    std::string cliente;
    std::string servico;
    float valor;
};

struct DespesaLocal
{
    std::string tipo;
    float valor;
};

class GerenciadorFinanceiro
{
private:
    std::vector<AgendamentoConcluido> historicoFaturamento;
    std::vector<DespesaLocal> listaDespesas;

public:
    GerenciadorFinanceiro() {}

    void AdicionarFaturamento(const std::string &cliente, const std::string &servico, float valor)
    {
        AgendamentoConcluido item;
        item.cliente = cliente;
        item.servico = servico;
        item.valor = valor;
        historicoFaturamento.push_back(item);
    }

    void AdicionarDespesa(const std::string &tipo, float valor)
    {
        DespesaLocal despesa;
        despesa.tipo = tipo;
        despesa.valor = valor;
        listaDespesas.push_back(despesa);
    }

    float CalcularTotalBruto() const
    {
        float total = 0.0f;
        for (const auto &item : historicoFaturamento)
        {
            total += item.valor;
        }
        return total;
    }

    float CalcularTotalDespesas() const
    {
        float total = 0.0f;
        for (const auto &item : listaDespesas)
        {
            total += item.valor;
        }
        return total;
    }

    float CalcularLucroLiquido() const
    {
        return CalcularTotalBruto() - CalcularTotalDespesas();
    }

    const std::vector<AgendamentoConcluido> &ObterHistorico() const
    {
        return historicoFaturamento;
    }

    const std::vector<DespesaLocal> &ObterDespesas() const
    {
        return listaDespesas;
    }
};

#endif