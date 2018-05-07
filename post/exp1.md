# 实验一 - 系统调用

PPT上步骤挺清楚的，这里补充一张系统调用执行的流程图，希望可以帮助理解。

（图片可能有点模糊，可以放大点看）

![Syscall process](https://github.com/AndreamApp/epos/raw/master/screenshots/syscall_process.png)

其中每个方框表示一个调用过程，第一行是函数所在文件，之后是相关的代码。

灰色的是不需要我们修改的，只是展示调用流程。黑色的是需要我们修改的。

可以按照系统调用的流程来实现系统调用，会更容易理解一些。
