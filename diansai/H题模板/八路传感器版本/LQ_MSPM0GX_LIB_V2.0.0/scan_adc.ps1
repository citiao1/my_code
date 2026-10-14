$file = 'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\LQ_Lib\LQ_Driver\LQ_adc.c'
$lines = Get-Content $file
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match 'initSingleSample|configConversionMem|startConversion|getMemResult|getStatus|isConversionCompleted|enablePower|enableConversions|isConversionStarted|getConversionMemConfig') {
        Write-Host ("$($i+1): " + $lines[$i])
    }
}
