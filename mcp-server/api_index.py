"""In-memory search index over UE Python API knowledge files.

Parses ~896 markdown files from ue-toolkit/skills/ue-api-*/knowledge/ at startup,
building a searchable index of type names, modules, inheritance, and descriptions.
"""

import re
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple


@dataclass
class TypeInfo:
    """Parsed UE Python API type."""
    name: str               # e.g. "ActorDesc"
    qualified: str           # e.g. "unreal.ActorDesc"
    module: str              # e.g. "Engine"
    skill_group: str         # e.g. "ue-api-core"
    inherits: str = ""       # e.g. "StructBase"
    header: str = ""         # e.g. "WorldPartitionBlueprintLibrary.h"
    description: str = ""    # First paragraph after the heading
    kind: str = "class"      # class, enum, struct (inferred from inherits)
    properties: list = field(default_factory=list)
    methods: list = field(default_factory=list)
    raw_section: str = ""    # Full markdown section for detailed view


# Regex patterns for parsing knowledge .md files
_RE_MODULE_HEADER = re.compile(
    r"^#\s+UE Python API\s+—\s+(\S+)\s+Module", re.MULTILINE
)
_RE_TYPE_COUNT = re.compile(r"\*\*(\d+)\s+types?\*\*")
_RE_TYPE_HEADING = re.compile(r"^###\s+`unreal\.(\w+)`", re.MULTILINE)
_RE_INHERITS = re.compile(r"Inherits:\s+`(\w+)`")
_RE_HEADER_FILE = re.compile(r"Header:\s+`([^`]+)`")
_RE_PROP_LINE = re.compile(r"^\s+-\s+`(\w+)`:\s+`([^`]+)`.*?—\s*(.*)", re.MULTILINE)
_RE_METHOD_LINE = re.compile(
    r"^\s+-\s+`(\w+)\(([^)]*)\)`\s*(?:->\s*`([^`]+)`)?\s*(?:\[([^\]]+)\])?\s*—\s*(.*)",
    re.MULTILINE,
)


class ApiIndex:
    """In-memory search index over UE API knowledge files."""

    def __init__(self, skills_dir: Path):
        self._types: Dict[str, TypeInfo] = {}          # name -> TypeInfo
        self._types_lower: Dict[str, str] = {}         # lowercase name -> name
        self._modules: Dict[str, List[str]] = {}       # module -> [type names]
        self._module_files: Dict[str, Path] = {}       # module -> file path
        self._all_modules: List[str] = []
        self._load(skills_dir)

    def _load(self, skills_dir: Path):
        """Parse all knowledge .md files from ue-api-* skill directories."""
        api_dirs = sorted(skills_dir.glob("ue-api-*/knowledge"))
        for knowledge_dir in api_dirs:
            skill_group = knowledge_dir.parent.name
            for md_file in sorted(knowledge_dir.glob("*.md")):
                self._parse_file(md_file, skill_group)

        self._all_modules = sorted(self._modules.keys())

    def _parse_file(self, path: Path, skill_group: str):
        """Parse a single knowledge .md file into TypeInfo objects."""
        text = path.read_text(errors="replace")

        # Extract module name from header
        m = _RE_MODULE_HEADER.search(text)
        module = m.group(1) if m else path.stem
        self._module_files[module] = path

        # Split into per-type sections on ### headings
        sections = re.split(r"(?=^###\s+`unreal\.)", text, flags=re.MULTILINE)

        type_names = []
        for section in sections:
            tm = _RE_TYPE_HEADING.search(section)
            if not tm:
                continue

            name = tm.group(1)
            qualified = f"unreal.{name}"

            # Parse metadata line (Inherits: ... | Header: ...)
            inherits = ""
            header = ""
            im = _RE_INHERITS.search(section)
            if im:
                inherits = im.group(1)
            hm = _RE_HEADER_FILE.search(section)
            if hm:
                header = hm.group(1)

            # Description: text between metadata line and first **Properties**/**Methods**
            desc_lines = []
            in_desc = False
            for line in section.split("\n"):
                if line.startswith("###"):
                    continue
                if "Inherits:" in line and "Header:" in line:
                    in_desc = True
                    continue
                if in_desc:
                    if line.startswith("**Properties**") or line.startswith("**Methods**"):
                        break
                    stripped = line.strip()
                    if stripped:
                        desc_lines.append(stripped)
            description = " ".join(desc_lines)[:300]

            # Infer kind
            kind = "class"
            if inherits == "EnumBase":
                kind = "enum"
            elif inherits == "StructBase":
                kind = "struct"

            # Parse properties
            properties = []
            for pm in _RE_PROP_LINE.finditer(section):
                properties.append({
                    "name": pm.group(1),
                    "type": pm.group(2),
                    "description": pm.group(3).strip()[:200],
                })

            # Parse methods
            methods = []
            for mm in _RE_METHOD_LINE.finditer(section):
                methods.append({
                    "name": mm.group(1),
                    "params": mm.group(2),
                    "returns": mm.group(3) or "",
                    "tags": mm.group(4) or "",
                    "description": mm.group(5).strip()[:200],
                })

            info = TypeInfo(
                name=name,
                qualified=qualified,
                module=module,
                skill_group=skill_group,
                inherits=inherits,
                header=header,
                description=description,
                kind=kind,
                properties=properties,
                methods=methods,
                raw_section=section.strip(),
            )
            self._types[name] = info
            self._types_lower[name.lower()] = name
            type_names.append(name)

        if type_names:
            self._modules.setdefault(module, []).extend(type_names)

    # ── Search API ──────────────────────────────────────────────────────

    def search(self, query: str, limit: int = 10) -> List[dict]:
        """Search types by name, module, or keyword. Returns summaries."""
        query_lower = query.lower()
        scored: List[Tuple[int, TypeInfo]] = []

        for info in self._types.values():
            score = 0
            name_lower = info.name.lower()

            # Exact match
            if name_lower == query_lower:
                score = 1000
            # Prefix match
            elif name_lower.startswith(query_lower):
                score = 500
            # Substring in name
            elif query_lower in name_lower:
                score = 300
            # Module match
            elif query_lower == info.module.lower():
                score = 200
            elif query_lower in info.module.lower():
                score = 100
            # Keyword in description
            elif query_lower in info.description.lower():
                score = 50
            # Keyword in method/property names
            else:
                for method in info.methods:
                    if query_lower in method["name"].lower():
                        score = 30
                        break
                if score == 0:
                    for prop in info.properties:
                        if query_lower in prop["name"].lower():
                            score = 20
                            break

            if score > 0:
                scored.append((score, info))

        scored.sort(key=lambda x: (-x[0], x[1].name))
        return [self._summary(info) for _, info in scored[:limit]]

    def get_type(self, name: str) -> Optional[dict]:
        """Get full reference for a type by exact or case-insensitive name."""
        info = self._types.get(name)
        if not info:
            canonical = self._types_lower.get(name.lower())
            if canonical:
                info = self._types.get(canonical)
        if not info:
            return None
        return self._detail(info)

    def get_module(self, module: str) -> Optional[dict]:
        """List all types in a module with summaries."""
        type_names = self._modules.get(module)
        if not type_names:
            # Case-insensitive fallback
            for mod_name, names in self._modules.items():
                if mod_name.lower() == module.lower():
                    type_names = names
                    module = mod_name
                    break
        if not type_names:
            return None
        types = []
        for name in sorted(type_names):
            info = self._types.get(name)
            if info:
                types.append(self._summary(info))
        return {
            "module": module,
            "type_count": len(types),
            "types": types,
        }

    def list_modules(self) -> List[str]:
        """List all available module names."""
        return self._all_modules

    # ── Formatting ──────────────────────────────────────────────────────

    def _summary(self, info: TypeInfo) -> dict:
        return {
            "name": info.qualified,
            "module": info.module,
            "kind": info.kind,
            "inherits": info.inherits,
            "description": info.description[:150],
            "properties": len(info.properties),
            "methods": len(info.methods),
        }

    def _detail(self, info: TypeInfo) -> dict:
        return {
            "name": info.qualified,
            "module": info.module,
            "kind": info.kind,
            "inherits": info.inherits,
            "header": info.header,
            "description": info.description,
            "properties": info.properties,
            "methods": info.methods,
        }
