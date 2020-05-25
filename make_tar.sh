mkdir temp
cd temp
cp -R ../bin bin
cp -R ../base base
cp -R ../server server
cp -R ../solver solver
cp -R ../test test
cp -R ../models models
cp -R ../third_party third_party
cp ../README.md README.md
cp ../CMakeLists.txt CMakeLists.txt
cp ../deb-packages.txt deb-packages.txt
cp ../build_zip.sh build_zip.sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ..
mv build/bin/tgnews tgnews
chmod +x tgnews
cp tgnews ../tgnews
zip -r submission.zip bin test base server solver models third_party README.md CMakeLists.txt deb-packages.txt tgnews build_zip.sh
mv submission.zip ../submission.zip
cd ..
rm -rf temp
