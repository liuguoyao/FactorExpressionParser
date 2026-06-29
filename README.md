# FactorExpressionParser

一个纯 C++11 的金融因子表达式解析器。将字符串形式的因子表达式解析为抽象语法树（AST）。

## 快速开始

```bash
# 构建
cmake -B build -S .
cmake --build build

# 运行
./build/Debug/fep.exe
```

### 运行测试（可选）

```bash
cmake -B build -S . -DBUILD_TESTS=ON
cmake --build build
./build/tests/Debug/test_fep.exe
```

## 表达式语言

所有运算定义在一个三维数组 `data[时间][股票][字段]` 上。

### 逐元素运算符

| 类别 | 运算符/函数 |
|------|-------------|
| 算术 | `+` `-` `*` `/` `^`（幂） |
| 比较 | `<` `>` `<=` `>=` `==` `!=` |
| 三元 | `? :` |
| 函数 | `abs(x)` `log(x)` `sign(x)` `signedpower(x,a)` `exp(x)` `sqrt(x)` |

### 时间序列函数（沿股票时间轴纵向计算）

| 函数 | 说明 |
|------|------|
| `delay(x, d)` | x 滞后 d 个时间点 |
| `delta(x, d)` | x - delay(x, d) |
| `ts_sum(x, d)` | 过去 d 天滚动和 |
| `ts_mean(x, d)` | 过去 d 天滚动均值 |
| `ts_std(x, d)` | 滚动标准差 |
| `ts_corr(x, y, d)` | 滚动相关系数 |

### 横截面函数（同一时间点所有股票上计算）

| 函数 | 说明 |
|------|------|
| `rank(x)` | 排名，缩放到 0~1 |
| `scale(x, a=1)` | 缩放使 sum\|x\|=a |
| `zscore(x)` | 标准化 |
| `group_rank(x, sector)` | 行业内排名 |
| `ind_neutralize(x, sector)` | 行业中性化 |

### 示例

```
输入：rank(ts_mean(close, 5) / delay(close, 10))
输出：rank((ts_mean(close, 5) / delay(close, 10)))

输入：close + volume * 2
输出：(close + (volume * 2))

输入：close >= open ? close : open
输出：((close >= open) ? close : open)
```

## 项目结构

```
├── CMakeLists.txt          # 构建配置（C++11）
├── README.md
├── src/
│   ├── fepCore.h/cpp       # 公开 API: fep(string)
│   ├── Token.h             # 词法 Token 定义（17 种类型）
│   ├── Lexer.h/cpp         # 词法分析器
│   ├── Expr.h              # AST 节点（6 种节点类型，unique_ptr 管理）
│   ├── Parser.h/cpp        # Pratt 解析器（7 级优先级）
│   └── main.cpp            # 示例入口
├── tests/
│   ├── CMakeLists.txt      # 测试构建（链接 fep_core 静态库）
│   ├── test_lexer.cpp      # 词法分析测试（12 用例）
│   └── test_parser.cpp     # 解析器测试（19 用例）
└── third_party/Catch2/     # Catch2 v3 测试框架
```

## 架构设计

### 三段式流水线

```
输入字符串 → [Lexer] → Token 流 → [Parser] → AST
```

1. **词法分析**（Lexer）—— 逐字符扫描，识别数字、标识符、运算符、括号、逗号。遇到非法字符立即报错。
2. **语法分析**（Parser）—— Pratt 解析器（TDOP），7 级运算符优先级：
   - `? :`（三元）< 比较 < `+ -` < `* /` < `^`（右结合）< 一元 < 函数调用
3. **AST 输出** —— 通过 `to_string()` 序列化为 S 表达式，方便调试与下游处理。

### 关键设计决策

- **Pratt 解析**：每个 Token 类型独立注册前缀/中缀解析函数，新增运算符只需添加枚举值和绑定优先级，无需修改解析流程。
- **`std::unique_ptr` 管理**：所有 AST 子节点通过 `unique_ptr` 持有，无需手动 delete，所有权清晰。
- **错误信息**：每个异常携带源码位置（`pos`），定位精确。
- **源码与测试解耦**：核心代码编译为 `fep_core` 静态库，测试目标和主程序各自链接，互不依赖。

## 依赖

- C++11 标准库（无第三方依赖）
- 测试框架：Catch2 v3（仅 `-DBUILD_TESTS=ON` 时需要）

## 测试覆盖

31 个测试用例，66 个断言，覆盖：

- 数字/浮点/标识符/运算符/括号/逗号 词法识别
- 非法字符/单 `=` /单 `!` 错误处理
- 运算符优先级与左/右结合性
- 括号分组、一元正负、函数调用（0~n 参数）、嵌套调用
- 三元表达式、比较链
- 空输入/括号不匹配/尾随垃圾 错误处理

## License

MIT
