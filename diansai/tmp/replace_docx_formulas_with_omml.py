from copy import deepcopy
from pathlib import Path
from tempfile import NamedTemporaryFile
from zipfile import ZIP_DEFLATED, ZipFile

from lxml import etree


TARGET = Path(
    r"D:\my_code\my_code\diansai\H题模板\2026电赛报告H题_MaixCAM方案_底盘与机械机构阶段初稿_排版校正版.docx"
)
FORMULA_SOURCE = Path(r"D:\my_code\my_code\diansai\tmp\formulas_omml_source.docx")
OUTPUT = Path(
    r"D:\my_code\my_code\diansai\H题模板\2026电赛报告H题_MaixCAM方案_底盘与机械机构阶段初稿_公式优化版.docx"
)

NS = {
    "w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
    "m": "http://schemas.openxmlformats.org/officeDocument/2006/math",
}

FORMULA_TEXTS = [
    "e_y = sat[-1,1]{ [Σ(w_i b_i) / Σb_i] / 3.5 }",
    "ω_d(k) = sat{ K_p,e e_y(k) + K_d,e [e_y(k)-e_y(k-1)]/T_s }",
    "Δv = K_p,ω(ω_d-ω_z) + K_i,ω∫(ω_d-ω_z)dt",
    "v_L* = v_0 + 0.4Δv，    v_R* = v_0 - 0.4Δv",
    "Δu(k)=K_p[e(k)-e(k-1)] + K_i e(k)T_s + K_d[e(k)-2e(k-1)+e(k-2)]/T_s",
    "h(φ) ≈ r[sin(φ+φ_0)-sinφ_0]，    θ(φ)=arctan[h(φ)/L]≈h(φ)/L",
    "e(k)=x_r-x(k)",
    "ė_f(k)=LPF{[e(k)-e(k-1)]/T_k}",
    "θ_d(k)=sat[-θ_max,θ_max]{ s[K_p e(k)+K_d ė_f(k)] }",
]


def paragraph_text(paragraph):
    return "".join(paragraph.xpath(".//w:t/text()", namespaces=NS))


with ZipFile(FORMULA_SOURCE) as source_zip:
    source_xml = etree.fromstring(source_zip.read("word/document.xml"))
formula_nodes = source_xml.xpath("//w:p/m:oMathPara", namespaces=NS)
if len(formula_nodes) != len(FORMULA_TEXTS):
    raise RuntimeError(
        f"Expected {len(FORMULA_TEXTS)} generated formulas, found {len(formula_nodes)}"
    )

with ZipFile(TARGET) as target_zip:
    document_xml = etree.fromstring(target_zip.read("word/document.xml"))
    target_entries = {item.filename: target_zip.read(item.filename) for item in target_zip.infolist()}

paragraphs = document_xml.xpath("//w:p", namespaces=NS)
replaced = []
for formula_text, formula_node in zip(FORMULA_TEXTS, formula_nodes):
    matches = [p for p in paragraphs if paragraph_text(p) == formula_text]
    if len(matches) != 1:
        raise RuntimeError(f"Expected one paragraph for {formula_text!r}, found {len(matches)}")
    paragraph = matches[0]
    for child in list(paragraph):
        if child.tag != f"{{{NS['w']}}}pPr":
            paragraph.remove(child)
    paragraph_properties = paragraph.find("w:pPr", namespaces=NS)
    spacing = paragraph_properties.find("w:spacing", namespaces=NS)
    if spacing is None:
        spacing = etree.SubElement(paragraph_properties, f"{{{NS['w']}}}spacing")
    spacing.set(f"{{{NS['w']}}}before", "0")
    spacing.set(f"{{{NS['w']}}}after", "40")
    paragraph.append(deepcopy(formula_node))
    replaced.append(formula_text)

target_entries["word/document.xml"] = etree.tostring(
    document_xml,
    xml_declaration=True,
    encoding="UTF-8",
    standalone="yes",
)

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
with NamedTemporaryFile(dir=OUTPUT.parent, suffix=".docx", delete=False) as temp_file:
    temp_path = Path(temp_file.name)
try:
    with ZipFile(temp_path, "w", ZIP_DEFLATED) as output_zip:
        for filename, data in target_entries.items():
            output_zip.writestr(filename, data)
    temp_path.replace(OUTPUT)
finally:
    if temp_path.exists():
        temp_path.unlink()

print(f"OUTPUT={OUTPUT}")
print(f"REPLACED={len(replaced)}")
