from __future__ import annotations

from typing import TYPE_CHECKING
from typing import List

if TYPE_CHECKING:
    from jsonpath24 import FilterFunction
    from jsonpath24 import JSONPathNode
    from jsonpath24 import Segments


from jsonpath24 import Env_
from jsonpath24 import FunctionExtensionMap
from jsonpath24 import FunctionExtensionTypes
from jsonpath24 import FunctionSignatureMap

from ._nothing import NOTHING
from ._path import JSONPath
from .functions import Count
from .functions import Length
from .functions import Match
from .functions import Search
from .functions import Value


class JSONPathEnvironment:
    __slots__ = ("_function_register", "_function_signatures", "_env")

    def __init__(self) -> None:
        self._function_register = FunctionExtensionMap()
        self._function_signatures = FunctionSignatureMap()
        self.setup_function_register()
        self._env = Env_(
            self._function_register,
            self._function_signatures,
            NOTHING,
        )

    def register_function(self, name: str, func: FilterFunction) -> None:
        self._function_register[name] = func
        self._function_signatures[name] = FunctionExtensionTypes(
            list(func.arg_types), func.return_type
        )
        self._env = Env_(
            self._function_register,
            self._function_signatures,
            NOTHING,
        )

    def setup_function_register(self) -> None:
        """Initialize function extensions."""
        self.register_function("count", Count())
        self.register_function("length", Length())
        self.register_function("match", Match())
        self.register_function("search", Search())
        self.register_function("value", Value())

    def compile(self, path: str) -> JSONPath:  # noqa: A003
        return JSONPath(self, self._env.parse(path))

    def findall(self, path: str, data: object) -> List[object]:
        return [node.value for node in self.query(path, data)]

    def query(self, path: str, data: object) -> List[JSONPathNode]:
        return self._env.query(path, data)

    def from_segments(self, segments: Segments, data: object) -> List[JSONPathNode]:
        return self._env.from_segments(segments, data)
