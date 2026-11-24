# 02：从 C++ 到几何、拓扑和视图

## 1. 最小前置知识

你需要能阅读类、构造函数、继承、虚函数、引用、模板、lambda 和 try/catch。Qt 先练习一个按钮连接 lambda 后弹窗，再进入三维部分。

本项目没有使用 Qt Designer，也没有给 CadView 加 Q_OBJECT，因为它没有自定义信号、槽或元对象属性；常规连接和 QWidget 事件重写不要求它。

## 2. 数据流

```text
main.cpp 的按钮/参数
       ↓
modeling.cpp：算法生成 TopoDS_Shape
       ↓
CadView::displayShape：验证、包装 AIS_Shape
       ↓
AIS_InteractiveContext：管理显示与选择
       ↓
V3d_View → 原生 Qt 窗口 → OpenGL 图像
```

`modeling.cpp` 不依赖 Qt，所以可以没有窗口地执行几何测试。先验证模型，再研究它如何被显示，排错会更容易。

## 3. 几何和拓扑

| 数学描述 | 常见类型 | 有边界与连接关系的描述 | 常见类型 |
|---|---|---|---|
| 点 | gp_Pnt | 顶点 | TopoDS_Vertex |
| 曲线 | Geom_Curve | 边 | TopoDS_Edge |
| 曲面 | Geom_Surface | 面 | TopoDS_Face |

平面在数学上无限延伸，长方体的一个矩形面则由边界限定。边还可有曲面参数域中的二维表示（pcurve）；后续做复杂曲面时，不能把一条三维曲线看成面边界的全部信息。

拓扑类型包括 Vertex、Edge、Wire、Face、Shell、Solid、CompSolid、Compound。Wire 可以开也可以闭；Shell 可以开也可以闭。Compound 是形状集合，不自动表示融合成一个实体。

`TopoDS_Shape` 能包装各种拓扑类型；检查 `ShapeType()` 后再转换。复制 TopoDS_Shape 包装对象可能共享底层数据，普通赋值不是深拷贝。

## 4. 坐标与变换

```cpp
gp_Pnt location(10, 20, 30); // 位置
gp_Vec delta(0, 0, 50);     // 位移向量，有长度
gp_Dir axis(0, 0, 1);      // 单位方向，不能是零向量
```

本课程采用全局 XYZ；角度使用弧度；构建尺寸约定为 mm。1 rad 不是 1 degree，90 度应写成 π/2。

## 5. Handle 与生命周期

OCCT 7.9 用引用计数指针管理部分对象：

```cpp
Handle(AIS_Shape) presentation = new AIS_Shape(shape);
```

不需要手工 delete。gp_Pnt、TopoDS_Shape 等通常作为值对象使用。不要把 Handle 理解成可以包装任意类的通用指针。

窗口销毁时先移除显示对象和视图，再让 Qt 释放原生窗口。本工程析构函数按此顺序处理，防止视图继续引用已销毁的窗口。

## 6. 三种对象变化

- 修改 TopoDS_Shape：模型数据变化。
- 修改 AIS_Shape：显示包装变化，需 Redisplay 或更新上下文。
- 修改 Camera：观察方式变化，不改变实体体积。

旋转视图不是旋转模型；用 gp_Trsf 旋转模型才会改变它在世界坐标中的位置。

## 7. 算法结果检查

```text
参数检查 → 执行算法 → IsDone → Shape 非空 → BRepCheck → 数值/拓扑验收
```

IsDone 只说明算法完成。一个合法的结果也可能尺寸不对、数量不对或没有真正打孔。因此课程同时检查体积、实体数和截图。

公差不是越小越好，更不是遇到问题就放大。先检查尺寸、相交、轮廓闭合和拓扑，再研究与模型尺度一致的公差设置。

## 8. 阅读顺序与练习

先读 modeling.h 的接口；跟进 box、plate；再读 main 的按钮连接和 CadView::displayShape。最后阅读鼠标事件与文件交换。

练习：说明为什么“模型能显示”不等于“模型是一个封闭实体”；为什么“变红”不等于“几何改变”；为什么把两个实体放入 Compound 不等于布尔并集。
