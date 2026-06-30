#include <windows.h>
#include <stdio.h>
#include "profit_analysis.h"  
#include <math.h>

static CategoryProfit* g_profits = NULL;
static int g_count = 0;
static MonthlyProfit* g_monthlyProfits = NULL;
static int g_monthlyCount = 0;
static CategoryProfit* g_pieProfits = NULL;
static int g_pieCount = 0;
static COLORREF extendedColors[] = {
    RGB(255, 99, 132),    // 亮红
    RGB(54, 162, 235),    // 亮蓝
    RGB(255, 206, 86),    // 亮黄
    RGB(75, 192, 192),    // 青绿
    RGB(153, 102, 255),   // 紫色
    RGB(255, 159, 64),    // 橙色
    RGB(231, 233, 237),   // 浅灰
    RGB(108, 117, 125),   // 中灰
    RGB(220, 53, 69),     // 深红
    RGB(28, 113, 216),    // 深蓝
    RGB(25, 135, 84),     // 深绿
    RGB(13, 110, 253)     // 靛蓝
};
static int colorCount = sizeof(extendedColors) / sizeof(COLORREF); 
// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 绘图区域尺寸
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int margin = 50;  // 边距

        // 1. 绘制白色背景
        HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rect, hWhiteBrush);
        DeleteObject(hWhiteBrush);

        if (g_count <= 0 || g_profits == NULL) {
            EndPaint(hwnd, &ps);
            return 0;
        }

        //绘制坐标轴
        HPEN hBlackPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        SelectObject(hdc, hBlackPen);
        // X轴
        MoveToEx(hdc, margin, height - margin, NULL);
        LineTo(hdc, width - margin, height - margin);
        // Y轴
        MoveToEx(hdc, margin, margin, NULL);
        LineTo(hdc, margin, height - margin);
        DeleteObject(hBlackPen);

        //计算柱状图参数
        int barWidth = 40;  // 柱子宽度
        int totalBarWidth = g_count * barWidth;
        int spacing = (width - 2 * margin - totalBarWidth) / (g_count + 1);  // 间距
        int startX = margin + spacing;

        // 找到最大利润
        float maxProfit = 0;
        for (int i = 0; i < g_count; i++) {
            if (g_profits[i].profit > maxProfit) {
                maxProfit = g_profits[i].profit;
            }
        }
        if (maxProfit <= 0) maxProfit = 1;  

        // 4. 绘制柱子（蓝色）和文字
        HBRUSH hBlueBrush = CreateSolidBrush(RGB(0, 0, 255));
        HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));  // 柱子边框
        SelectObject(hdc, hBlueBrush);
        SelectObject(hdc, hBorderPen);

        // 设置文字字体
        HFONT hFont = CreateFont(
            12, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            GB2312_CHARSET, OUT_DEFAULT_PRECIS,  
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_MODERN,
            L"宋体"
        );
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(0, 0, 0));  // 黑色文字
        SetBkColor(hdc, RGB(255, 255, 255));  // 白色背景

        for (int i = 0; i < g_count; i++) {
            // 计算柱子高度
            int barHeight = (int)((g_profits[i].profit / maxProfit) * (height - 2 * margin - 50));
            int x = startX + i * (barWidth + spacing);
            int y = height - margin - barHeight;  

            // 绘制柱子
            Rectangle(hdc, x, y, x + barWidth, height - margin);

            // 绘制商品名称
            char brandText[30];
            sprintf(brandText, "%s", g_profits[i].brand);
            // 多字节转宽字符
            wchar_t wBrandText[30];
            MultiByteToWideChar(CP_ACP, 0, brandText, -1, wBrandText, 30);
            // 使用宽字符版本TextOutW输出
            TextOutW(hdc, x, height - margin + 10, wBrandText, wcslen(wBrandText));

            // 绘制利润值
            char profitText[30];
            sprintf(profitText, "%.2f", g_profits[i].profit);
            // 多字节转宽字符
            wchar_t wProfitText[30];
            MultiByteToWideChar(CP_ACP, 0, profitText, -1, wProfitText, 30);
            TextOutW(hdc, x, y - 20, wProfitText, wcslen(wProfitText));
        }

        // 释放资源
        DeleteObject(hBlueBrush);
        DeleteObject(hBorderPen);
        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 创建窗口并显示图表
void showProfitChart(CategoryProfit* profits, int count, const wchar_t* windowTitle) {
    
    g_profits = profits;
    g_count = count;

    //注册窗口类
    const char* className = L"ProfitChartClass";
    WNDCLASSW wc = { 0 };  
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = className;  
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);  

    //创建窗口
    HWND hwnd = CreateWindowExW(  
        0,
        L"ProfitChartClass",
        windowTitle,  
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    if (hwnd == NULL) return;

    //显示窗口
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    //消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnregisterClass(className, GetModuleHandle(NULL));
    g_profits = NULL;
    g_count = 0;
}

LRESULT CALLBACK MonthlyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 绘图区域尺寸
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int margin = 50;  // 边距

        //绘制白色背景
        HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rect, hWhiteBrush);
        DeleteObject(hWhiteBrush);

        if (g_monthlyCount <= 0 || g_monthlyProfits == NULL) {
            EndPaint(hwnd, &ps);
            return 0;
        }

        //绘制坐标轴
        HPEN hBlackPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        SelectObject(hdc, hBlackPen);
        // X轴
        MoveToEx(hdc, margin, height - margin, NULL);
        LineTo(hdc, width - margin, height - margin);
        // Y轴
        MoveToEx(hdc, margin, margin, NULL);
        LineTo(hdc, margin, height - margin);
        DeleteObject(hBlackPen);

        //准备柱状图数据
        int barWidth = 40;  // 柱子宽度
        int totalBarWidth = g_monthlyCount * barWidth;
        int spacing = (width - 2 * margin - totalBarWidth) / (g_monthlyCount + 1);  // 间距
        int startX = margin + spacing;

        // 找到最大利润值用于Y轴缩放
        float maxProfit = 0;
        for (int i = 0; i < g_monthlyCount; i++) {
            if (g_monthlyProfits[i].profit > maxProfit) {
                maxProfit = g_monthlyProfits[i].profit;
            }
        }
        if (maxProfit <= 0) maxProfit = 1; 

        //绘制柱子和文本
        HBRUSH hGreenBrush = CreateSolidBrush(RGB(0, 128, 0));  // 用绿色区分月度图表
        HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));  // 柱子边框
        SelectObject(hdc, hGreenBrush);
        SelectObject(hdc, hBorderPen);

        // 创建字体
        HFONT hFont = CreateFont(
            12, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            GB2312_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_MODERN,
            L"宋体"
        );
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkColor(hdc, RGB(255, 255, 255));

        for (int i = 0; i < g_monthlyCount; i++) {
            // 计算柱子高度
            int barHeight = (int)((g_monthlyProfits[i].profit / maxProfit) * (height - 2 * margin - 50));
            int x = startX + i * (barWidth + spacing);
            int y = height - margin - barHeight;

            // 绘制柱子
            Rectangle(hdc, x, y, x + barWidth, height - margin);

            // 显示月份
            char monthText[8];
            sprintf(monthText, "%s", g_monthlyProfits[i].month);
            wchar_t wMonthText[8];
            MultiByteToWideChar(CP_ACP, 0, monthText, -1, wMonthText, 8);
            TextOutW(hdc, x, height - margin + 10, wMonthText, wcslen(wMonthText));

            // 显示利润值
            char profitText[30];
            sprintf(profitText, "%.2f", g_monthlyProfits[i].profit);
            wchar_t wProfitText[30];
            MultiByteToWideChar(CP_ACP, 0, profitText, -1, wProfitText, 30);
            TextOutW(hdc, x, y - 20, wProfitText, wcslen(wProfitText));
        }

        // 释放资源
        DeleteObject(hGreenBrush);
        DeleteObject(hBorderPen);
        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 显示月度利润图表
void showMonthlyProfitChart(MonthlyProfit* profits, int count) {
    
    g_monthlyProfits = profits;
    g_monthlyCount = count;

    //注册窗口类
    const wchar_t* className = L"MonthlyProfitChartClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = MonthlyWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    //创建窗口
    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"月度利润图表",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    if (hwnd == NULL) return;

    //显示窗口
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    //消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterClassW(className, GetModuleHandle(NULL));
    g_monthlyProfits = NULL;
    g_monthlyCount = 0;
}
// 商品库存柱状图回调函数
LRESULT CALLBACK GoodsStockWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static struct item* g_goods = NULL;  // 静态存储商品数据
    static int g_goodsCount = 0;

    // 初始化商品数据
    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        g_goods = (struct item*)pCreate->lpCreateParams;
        // 计算商品总数
        g_goodsCount = 0;
        while (g_goods[g_goodsCount].storage != -1) {
            g_goodsCount++;
        }
        return 0;
    }

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int margin = 50;  

        // 绘制白色背景
        HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rect, hWhiteBrush);
        DeleteObject(hWhiteBrush);

        if (g_goodsCount <= 0 || g_goods == NULL) {
            EndPaint(hwnd, &ps);
            return 0;
        }

        // 绘制坐标轴
        HPEN hBlackPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        SelectObject(hdc, hBlackPen);
        // X轴
        MoveToEx(hdc, margin, height - margin, NULL);
        LineTo(hdc, width - margin, height - margin);
        // Y轴
        MoveToEx(hdc, margin, margin, NULL);
        LineTo(hdc, margin, height - margin);
        DeleteObject(hBlackPen);

        // 计算柱状图参数
        int barWidth = 40;  // 柱子宽度
        int totalBarWidth = g_goodsCount * barWidth;
        int spacing = (width - 2 * margin - totalBarWidth) / (g_goodsCount + 1);  // 间距
        int startX = margin + spacing;

        // 找到最大库存值
        int maxStock = 0;
        for (int i = 0; i < g_goodsCount; i++) {
            if (g_goods[i].storage > maxStock) {
                maxStock = g_goods[i].storage;
            }
        }
        if (maxStock <= 0) maxStock = 1; 

        // 设置字体
        HFONT hFont = CreateFont(
            12, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            GB2312_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_MODERN,
            L"宋体"
        );
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkColor(hdc, RGB(255, 255, 255));

        // 绘制每个商品的柱子
        for (int i = 0; i < g_goodsCount; i++) {
            // 根据库存是否预警设置颜色
            HBRUSH hBarBrush = (g_goods[i].storage < 500 && g_goods[i].storage >= 0)
                ? CreateSolidBrush(RGB(255, 0, 0))  // 红色预警
                : CreateSolidBrush(RGB(0, 0, 255)); // 蓝色正常

            HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));  // 黑色边框
            SelectObject(hdc, hBarBrush);
            SelectObject(hdc, hBorderPen);

            // 计算柱子高度
            int barHeight = (int)((g_goods[i].storage * 1.0f / maxStock) * (height - 2 * margin - 50));
            int x = startX + i * (barWidth + spacing);
            int y = height - margin - barHeight;  // 柱子底部对齐X轴

            // 绘制柱子
            Rectangle(hdc, x, y, x + barWidth, height - margin);

            // 显示商品名称
            char brandText[30];
            sprintf(brandText, "%s", goods[i].brand);  
            wchar_t wBrandText[30];
            MultiByteToWideChar(CP_ACP, 0, brandText, -1, wBrandText, 30);
            TextOutW(hdc, x, height - margin + 10, wBrandText, wcslen(wBrandText));

            // 显示库存值
            char stockText[20];
            sprintf(stockText, "%d", g_goods[i].storage);
            wchar_t wStockText[20];
            MultiByteToWideChar(CP_ACP, 0, stockText, -1, wStockText, 20);
            TextOutW(hdc, x, y - 20, wStockText, wcslen(wStockText));
            DeleteObject(hBarBrush);
            DeleteObject(hBorderPen);
        }

       
        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 显示商品库存柱状图的入口函数
void showGoodsStockChart(struct item* goods, int count, const wchar_t* windowTitle) {
    struct item* goodsWithMarker = (struct item*)malloc((count + 1) * sizeof(struct item));
    memcpy(goodsWithMarker, goods, count * sizeof(struct item));
    goodsWithMarker[count].storage = -1; 
    // 注册窗口类
    const wchar_t* className = L"GoodsStockChartClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = GoodsStockWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowExW(
        0,
        className,
        windowTitle,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 600,  // 更宽的窗口以适应更多商品
        NULL, NULL, GetModuleHandle(NULL),
        goodsWithMarker  // 传递商品数据给窗口过程
    );
    if (hwnd == NULL) {
        free(goodsWithMarker);
        return;
    }

    // 显示窗口并进入消息循环
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理资源
    UnregisterClassW(className, GetModuleHandle(NULL));
    free(goodsWithMarker);
}


LRESULT CALLBACK PieChartWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // 填充白色背景
        HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rect, hWhiteBrush);
        DeleteObject(hWhiteBrush);

        if (g_pieCount <= 0 || g_pieProfits == NULL) {
            EndPaint(hwnd, &ps);
            return 0;
        }

        
        float totalProfit = 0;
        for (int i = 0; i < g_pieCount; i++) {
            totalProfit += g_pieProfits[i].profit;
        }
        if (totalProfit <= 0) {
            EndPaint(hwnd, &ps);
            return 0;
        }

        // 绘制饼图
        int pieSize = min(width, height) - 200;
        int x = (width - pieSize) / 2;
        int y = (height - pieSize) / 2;
        float startAngle = 0;

        // 设置支持中文的字体
        HFONT hFont = CreateFont(
            12, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            GB2312_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_MODERN,
            L"宋体"
        );
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkColor(hdc, RGB(255, 255, 255));

        for (int i = 0; i < g_pieCount; i++) {
            // 计算当前扇形角度
            float angle = (g_pieProfits[i].profit / totalProfit) * 360;
            int gdiAngle = (int)(angle * 10);
            int gdiStartAngle = (int)(startAngle * 10);

          
            HBRUSH hBrush = CreateSolidBrush(extendedColors[i % colorCount]);
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255)); 
            SelectObject(hdc, hBrush);
            SelectObject(hdc, hPen);
            // 绘制扇形（参数：设备上下文、外接矩形、起始点、结束点）
            Pie(hdc, x, y, x + pieSize, y + pieSize,
                x + pieSize / 2 + (int)(cos(startAngle * 3.14159 / 180) * pieSize / 2),
                y + pieSize / 2 - (int)(sin(startAngle * 3.14159 / 180) * pieSize / 2),
                x + pieSize / 2 + (int)(cos((startAngle + angle) * 3.14159 / 180) * pieSize / 2),
                y + pieSize / 2 - (int)(sin((startAngle + angle) * 3.14159 / 180) * pieSize / 2)
            );

            int legendX = x + pieSize + 50;
            int legendY = y + 50 + i * 30;
            // 绘制颜色块
            Rectangle(hdc, legendX, legendY, legendX + 20, legendY + 20);
            wchar_t wBrand[30];
            MultiByteToWideChar(CP_ACP, 0, g_pieProfits[i].brand, -1, wBrand, 30);
            int brandLen = wcslen(wBrand);
            TextOutW(hdc, legendX + 30, legendY, wBrand, brandLen);

            // 计算占比并格式化
            float percentage = (g_pieProfits[i].profit / totalProfit) * 100;
            char percentStr[20];
            sprintf(percentStr, " (%.1f%%)", percentage);
            // 转换占比字符串到宽字符
            wchar_t wPercent[20];
            MultiByteToWideChar(CP_ACP, 0, percentStr, -1, wPercent, 20);
            // 绘制占比
            TextOutW(hdc, legendX + 30 + brandLen * 12, legendY, wPercent, wcslen(wPercent));
            DeleteObject(hBrush);
            DeleteObject(hPen);

            startAngle += angle; // 更新起始角度
        }

        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 实现饼图显示函数
void showProfitPieChart(CategoryProfit* profits, int count, const wchar_t* windowTitle) {
    g_pieProfits = profits;
    g_pieCount = count;

    const wchar_t* className = L"PieChartClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = PieChartWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, className, windowTitle,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnregisterClass(className, GetModuleHandle(NULL));
    g_pieProfits = NULL;
    g_pieCount = 0;
}
// 月度折线图窗口过程
LRESULT CALLBACK MonthlyLineWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int margin = 50;

        // 绘制白色背景
        HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rect, hWhiteBrush);
        DeleteObject(hWhiteBrush);

        if (g_monthlyCount <= 0 || g_monthlyProfits == NULL) {
            EndPaint(hwnd, &ps);
            return 0;
        }

        // 绘制坐标轴
        HPEN hBlackPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        SelectObject(hdc, hBlackPen);
        // X轴
        MoveToEx(hdc, margin, height - margin, NULL);
        LineTo(hdc, width - margin, height - margin);
        // Y轴
        MoveToEx(hdc, margin, margin, NULL);
        LineTo(hdc, margin, height - margin);
        DeleteObject(hBlackPen);

        // 计算最大利润值
        float maxProfit = 0;
        for (int i = 0; i < g_monthlyCount; i++) {
            if (g_monthlyProfits[i].profit > maxProfit) {
                maxProfit = g_monthlyProfits[i].profit;
            }
        }
        if (maxProfit <= 0) maxProfit = 1;

        // 设置字体
        HFONT hFont = CreateFont(
            12, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            GB2312_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_MODERN,
            L"宋体"
        );
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkColor(hdc, RGB(255, 255, 255));

        // 计算点间距
        int pointSpacing = (width - 2 * margin) / (g_monthlyCount - 1);
        if (g_monthlyCount == 1) pointSpacing = 0;

        // 创建红色画笔用于绘制折线
        HPEN hLinePen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hLinePen);

        // 绘制折线
        for (int i = 0; i < g_monthlyCount; i++) {
            int x = margin + i * pointSpacing;
            int y = height - margin - (int)((g_monthlyProfits[i].profit / maxProfit) * (height - 2 * margin - 50));

            // 绘制数据点
            Ellipse(hdc, x - 5, y - 5, x + 5, y + 5);

            // 绘制连接线
            if (i > 0) {
                int prevX = margin + (i - 1) * pointSpacing;
                int prevY = height - margin - (int)((g_monthlyProfits[i - 1].profit / maxProfit) * (height - 2 * margin - 50));
                MoveToEx(hdc, prevX, prevY, NULL);
                LineTo(hdc, x, y);
            }

            // 绘制月份标签
            wchar_t wMonthText[20];
            MultiByteToWideChar(CP_ACP, 0, g_monthlyProfits[i].month, -1, wMonthText, 20);
            TextOutW(hdc, x - 15, height - margin + 10, wMonthText, wcslen(wMonthText));

            // 绘制利润值
            char profitText[30];
            sprintf(profitText, "%.2f", g_monthlyProfits[i].profit);
            wchar_t wProfitText[30];
            MultiByteToWideChar(CP_ACP, 0, profitText, -1, wProfitText, 30);
            TextOutW(hdc, x + 10, y - 10, wProfitText, wcslen(wProfitText));
        }
        SelectObject(hdc, hOldPen);
        DeleteObject(hLinePen);
        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 显示月度利润折线图
void showMonthlyLineChart(MonthlyProfit* profits, int count, const wchar_t* windowTitle) {
    g_monthlyProfits = profits;
    g_monthlyCount = count;

    const wchar_t* className = L"MonthlyLineChartClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = MonthlyLineWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        className,
        windowTitle,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    if (hwnd == NULL) return;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterClassW(className, GetModuleHandle(NULL));
    g_monthlyProfits = NULL;
    g_monthlyCount = 0;
}
