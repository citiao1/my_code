$ErrorActionPreference = 'Stop'
$word = $null
try {
  $word = New-Object -ComObject Word.Application
  $word.Visible = $false
  $doc = $word.Documents.Open('D:\my_code\my_code\作业\微机原理\必修-CSE22500E《微机原理及应用》电科2015秋期.doc')
  $doc.ExportAsFixedFormat('D:\my_code\my_code\作业\微机原理\试题与答案PDF汇总\必修-CSE22500E《微机原理及应用》电科2015秋期.pdf', 17)
  $doc.Close($false)
  $doc = $word.Documents.Open('D:\my_code\my_code\作业\微机原理\必修-CSE22500E《微机原理及应用》电科2015秋期_答案解析.docx')
  $doc.ExportAsFixedFormat('D:\my_code\my_code\作业\微机原理\试题与答案PDF汇总\必修-CSE22500E《微机原理及应用》电科2015秋期_答案解析.pdf', 17)
  $doc.Close($false)
} finally { if ($word -ne $null) { $word.Quit() } }