from pathlib import Path
import xml.etree.ElementTree as ET
from fontTools.ttLib import TTFont

base = Path(__file__).resolve().parent
fonts = base if base.name == "fonts" else base.joinpath("assets", "fonts")

main = fonts.joinpath("main.conf")
aliases = fonts.joinpath("aliases.conf")

tree = ET.parse(main)
root = tree.getroot()

groups = {}

for directory in [directory for directory in fonts.iterdir() if directory.is_dir()]:
    folder = directory.name
    groups[folder] = []

    for file in [file for file in directory.iterdir() if file.suffix.lower() in (".otf", ".ttf")]:
        records = TTFont(file)["name"].names
        name = next(
            (record.toUnicode() for record in records if record.nameID == 16 and record.isUnicode()),
            next((record.toUnicode() for record in records if record.nameID == 1 and record.isUnicode()), file.stem),
        )
        if name not in groups[folder]:
            groups[folder].append(name)

for folder, names in groups.items():
    if not names:
        continue
    node = ET.SubElement(root, "alias")
    ET.SubElement(node, "family").text = folder
    prefer = ET.SubElement(node, "prefer")
    for name in names:
        ET.SubElement(prefer, "family").text = name

ET.indent(tree, space="  ")
tree.write(aliases, encoding="utf-8", xml_declaration=True)