#PASSO: comando de espaçamento de movimentos
# Indica quantas jogadas esperar entre cada movimento.
PASSO 50
#POS: comando de colocação inicial do monstro (linha e coluna).
# Coordenadas baseadas no mapa 2.lvl (Row 5, Col 1)
POS 5 1
# Comandos de movimento executados em ciclo infinito
# 1. Move para Cima (W)
# 2. Move para a Direita (D)
# 3. PREPARA O CARREGAMENTO (C)
# 4. EXECUTA O CARREGAMENTO para a Direita (D) - vai correr até bater na parede
# 5. Espera 2 turnos (T 2)
# 6. Volta para a Esquerda (A)
W
D
C
D
T 2
A