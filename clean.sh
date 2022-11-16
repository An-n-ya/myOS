if [ ! -d "./out" ]; then
    # 如果out不存在就创建该文件夹
    mkdir out
fi

if [ -e "hd60M.img" ]; then
    rm -rf hd60M.img
fi

# 清除编译产物
rm -f out/*.bin out/*.o bochsout.txt