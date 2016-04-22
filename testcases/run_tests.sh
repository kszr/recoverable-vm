# Compile all tests
echo "Compiling basic.c";
g++ -std=c++11 basic.c ../rvm.cpp -o basic;
echo "Compiling multi.c";
g++ -std=c++11 multi.c ../rvm.cpp -o multi;
echo "Compiling abort.c";
g++ -std=c++11 abort.c ../rvm.cpp -o abort;
echo "Compiling multi-abort.c";
g++ -std=c++11 multi-abort.c ../rvm.cpp -o multi-abort;
echo "Compiling truncate.c";
g++ -std=c++11 truncate.c ../rvm.cpp -o truncate;
echo "Compiling test_deception.c";
g++ -std=c++11 test_deception.c ../rvm.cpp -o test_deception;
echo "Compiling out_of_order.c";
g++ -std=c++11 out_of_order.c ../rvm.cpp -o out_of_order;
echo "Compiling test_post_facto.c";
g++ -std=c++11 test_post_facto.c ../rvm.cpp -o test_post_facto;
echo "Compiling map_unmap.c";
g++ -std=c++11 map_unmap.c ../rvm.cpp -o map_unmap;
echo "Compiling truncate_threshold.cpp";
g++ -std=c++11 truncate_threshold.cpp ../rvm.cpp -o truncate_threshold;
echo ""

# Run all tests
echo "======= Running basic.c =======";
echo"";
./basic;
echo "";
echo "======= Running multi.c =======";
echo "";
./multi;
echo "";
echo "======= Running abort.c =======";
echo "";
./abort;
echo "";
echo "======= Running multi-abort.c =======";
echo "";
./multi-abort;
echo "";
echo "======= Running truncate.c =======";
echo "";
./truncate;
echo "";
echo "======= Running test_deception.c =======";
echo "";
./test_deception;
echo "";
echo "======= Running out_of_order.c =======";
echo "";
./out_of_order;
echo "======= Running test_post_facto.c =======";
echo "";
./test_post_facto
echo "======= Running map_unmap.c =======";
echo "";
./map_unmap
echo "======= Running truncate_threshold.cpp =======";
echo "";
./truncate_threshold