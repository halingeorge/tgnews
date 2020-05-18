# tgnews

```
sudo apt install libboost-all-dev

git submodule update --init
mkdir build
cmake .. && make -j4
bin/functional_test

cmake .. && make -j4; bin/tgnews server 12345 —log_to_stderr  // запустить сервер на порту 12345 и выводить логи в stderr.

curl -H "Content-Type: text/html" -H "Cache-Control: 9" -I -X PUT -T "./input.txt" "localhost:12345" // выполнить команду PUT file и вывести http ответ сервера.

Testing cli mode:
./bin/tgnews language data_test/20191118/13 > lang_ans
./bin/tgnews news data_test/20191118/13 > lang_news
./bin/tgnews categories data_test/20191118/13 > lang_cat

```
