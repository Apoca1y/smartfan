@echo off
chcp 65001 >nul
echo ================================
echo   智能风扇系统 - 工程清理工具
echo ================================
echo.
echo 正在清理Keil工程临时文件...
echo.

REM 删除编译生成的文件
if exist "Objects\*.*" (
    del /q "Objects\*.*"
    echo [√] Objects目录已清理
) else (
    echo [!] Objects目录为空
)

if exist "Listings\*.*" (
    del /q "Listings\*.*"
    echo [√] Listings目录已清理
) else (
    echo [!] Listings目录为空
)

REM 删除Keil用户配置文件（保留工程文件）
if exist "*.uvguix.*" (
    del /q "*.uvguix.*"
    echo [√] Keil用户界面配置文件已删除
)

echo.
echo ================================
echo   清理完成！
echo ================================
echo.
echo 提示：现在可以用Keil MDK打开Project.uvprojx重新编译
echo.
pause
