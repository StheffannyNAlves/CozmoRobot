#ifndef BOTAO_H
#define BOTAO_H

#include <stdbool.h>

void botao_init(unsigned linha);
bool botao_esperar(unsigned ms);   // true se houve clique antes de ms esgotar

#endif