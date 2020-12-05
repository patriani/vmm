# vmm
Um simulador de memória virtual com substituição de página por FIFO e maneira randômica.

- Para compilar o código pode-se usar:
  
  $ gcc vmm.c -o <nome_que_desejar>

- Para executar:

  - P1: <nome_que_desejar>
  - P2: <nome_algoritmo> (minúsculo)
  - P3: <frequencia_clock> (inteiro)

  $ ./P1 P2 P3 < anomaly.dat 

Obs.: P2 deve assumir, obrigatoriamente: fifo ou random.

Recomendação: a frequência de clock não é utilizada para o random e nem para o algoritmo FIFO, porém, poode ser utilizada por alguma função implementada futuramente e, portanto, é um parâmetro obrigatório (10 é um bom número default).
