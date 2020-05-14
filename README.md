# tgnews

```
sudo apt install libboost-all-dev

git submodule update --init
mkdir build
cmake .. && make -j4

cmake .. && make -j4; bin/tgnews --port 12345 —log_to_stderr  // запустить сервер на порту 12345 и выводить логи в stderr.

curl -I -X PUT -T "./input.txt" "localhost:12345" // выполнить команду PUT file и вывести http ответ сервера.
```
