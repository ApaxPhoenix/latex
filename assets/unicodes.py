import re
import urllib.request

url = "https://raw.githubusercontent.com/latex3/unicode-math/master/unicode-math-table.tex"

request = urllib.request.Request(
    url,
    headers={'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}
)

try:
    with urllib.request.urlopen(request) as response:
        data = response.read().decode("utf-8")
except Exception as error:
    print(f"Error fetching data: {error}")
    exit(1)

mapping = {
    "ord": "Ordinary",
    "alpha": "Ordinary",
    "op": "Operator",
    "bin": "Binary",
    "rel": "Relation",
    "open": "Opening",
    "close": "Closing",
    "punct": "Punctuation",
    "accent": "Accent",
    "fence": "Ordinary",
    "over": "Accent",
    "under": "Accent",
}

pattern = r'\\UnicodeMathSymbol\s*\{\s*"([0-9A-Fa-f]+)\s*\}\s*\{\s*\\([a-zA-Z0-9_]+)\s*\}\s*\{\s*\\math([a-zA-Z]+)\s*\}'
matches = re.findall(pattern, data)

symbols = {}
for code, name, kind in matches:
    if name not in symbols:
        type_name = mapping.get(kind.lower(), "Ordinary")
        symbols[name] = (code.upper(), type_name)

with open("unicodes.gperf", "w", encoding="utf-8") as file:
    file.write("%{\n")
    file.write('#include "syntax/unicodes.hpp"\n')
    file.write("%}\n")
    file.write("struct Entry {\n")
    file.write("    const char* name;\n")
    file.write("    std::uint32_t codepoint;\n")
    file.write("    syntax::Unicodes::Type type;\n")
    file.write("};\n")
    file.write("%%\n")
    for name, (code, type_name) in sorted(symbols.items()):
        file.write(f"{name}, 0x{code}, syntax::Unicodes::Type::{type_name}\n")
    file.write("%%\n")