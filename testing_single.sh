sudo insmod NPHeap/npheap.ko
sudo chmod 777 /dev/npheap
sudo insmod kernel_module/tnpheap.ko
sudo chmod 777 /dev/tnpheap
./benchmark/benchmark 256 8192 1
rm -f *.log
sudo rmmod tnpheap
sudo rmmod npheap