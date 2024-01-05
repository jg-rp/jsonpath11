from __future__ import annotations

from typing import TYPE_CHECKING
from typing import List

from jsonpath24 import query_
from jsonpath24 import to_string

from ._nothing import NOTHING

if TYPE_CHECKING:
    from jsonpath24 import JSONPathEnvironment
    from jsonpath24 import JSONPathNode
    from jsonpath24 import Segments


class JSONPath:
    __slots__ = (
        "environment",
        "segments",
    )

    def __init__(self, environment: JSONPathEnvironment, segments: Segments) -> None:
        self.environment = environment
        self.segments = segments

    def findall(self, data: object) -> List[object]:
        return [node.value for node in self.query(data)]

    def query(self, data: object) -> List[JSONPathNode]:
        return query_(
            self.segments,
            data,
            self.environment._function_register,  # noqa: SLF001
            self.environment._function_signatures,  # noqa: SLF001
            NOTHING,
        )

    def __repr__(self) -> str:
        return f"<jsonpath24.JSONPath {to_string(self.segments)}>"
