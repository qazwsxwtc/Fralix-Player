#include "WriteToFile.h"
#include <iostream>
#include <chrono>

// 静态成员初始化需要在 .cpp 中（如果是在头文件中定义则不需要，但这里我们在 cpp 中实现单例逻辑）
// 由于我们使用局部静态变量方法 (Meyers' Singleton)，不需要额外的静态成员初始化
