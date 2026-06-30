明白了，问题出在 Mermaid 9.1.2 对节点标签中特殊字符（如 `(`、`)`）的解析有严格限制。以下是兼容 9.1.2 版本的修正流程图，将所有特殊字符用引号包裹或改用中文描述：

```mermaid
flowchart TD
    A([开始 advance]) --> B[skip_whitespace<br>跳过空白字符]
    B --> C{pos >= src.size?}
    C -->|是| D[返回 END Token]
    C -->|否| E[读取当前字符 c]

    E --> F{"c 是数字<br>或 c 为点且后跟数字?"}
    F -->|是| G[read_number<br>数字字面量]
    G --> Z([结束])

    F -->|否| H{"c 是字母或下划线?"}
    H -->|是| I[read_identifier_or_keyword<br>标识符或关键字]
    I --> Z

    H -->|否| J[记录起始位置 start, pos++]
    J --> K{"判断 c"}

    K -->|"+"| L1[PLUS]
    K -->|"-"| L2[MINUS]
    K -->|"*"| L3[STAR]
    K -->|"/"| L4[SLASH]
    K -->|"^"| L5[CARET]
    K -->|"("| L6[LPAREN]
    K -->|")"| L7[RPAREN]
    K -->|","| L8[COMMA]
    K -->|"?"| L9[QUESTION]
    K -->|":"| L10[COLON]

    K -->|"<"| M1{"下一字符是 = ?"}
    M1 -->|是| M2["LE <="]
    M1 -->|否| M3["LT <"]

    K -->|">"| N1{"下一字符是 = ?"}
    N1 -->|是| N2["GE >="]
    N1 -->|否| N3["GT >"]

    K -->|"="| O1{"下一字符是 = ?"}
    O1 -->|是| O2["EQ =="]
    O1 -->|否| O3["抛出异常: 意外的 ="]

    K -->|"!"| P1{"下一字符是 = ?"}
    P1 -->|是| P2["NE !="]
    P1 -->|否| P3["抛出异常: 意外的 !"]

    K -->|其他| Q["抛出异常: 意外字符"]

    L1 --> Z
    L2 --> Z
    L3 --> Z
    L4 --> Z
    L5 --> Z
    L6 --> Z
    L7 --> Z
    L8 --> Z
    L9 --> Z
    L10 --> Z
    M2 --> Z
    M3 --> Z
    N2 --> Z
    N3 --> Z
    O2 --> Z
    P2 --> Z
```

---

### 子流程：`skip_whitespace`

```mermaid
flowchart TD
    A([开始 skip_whitespace]) --> B{"pos < src.size 且<br>当前字符是空白?"}
    B -->|是| C[pos++]
    C --> B
    B -->|否| D([结束])
```

---

### 子流程：`read_number`

```mermaid
flowchart TD
    A([开始 read_number]) --> B[记录 start = pos]
    B --> C[跳过连续数字]
    C --> D{"pos 处是 . ?"}
    D -->|是| E[标记 is_double = true<br>pos++ 并跳过连续数字]
    D -->|否| F[截取 num = src.substr]
    E --> F
    F --> G["构造 NUMBER Token<br>value = stod(num)"]
    G --> H([返回 Token])
```

---

### 子流程：`read_identifier_or_keyword`

```mermaid
flowchart TD
    A([开始 read_identifier_or_keyword]) --> B[记录 start = pos]
    B --> C[跳过字母数字下划线]
    C --> D[截取 id = src.substr]
    D --> E[构造 IDENTIFIER Token]
    E --> F([返回 Token])
```

---

（兼容 Mermaid 9.1.2）
