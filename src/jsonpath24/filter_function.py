"""Base class for JSONPath function extensions."""
from __future__ import annotations

from abc import ABC
from abc import abstractmethod
from typing import TYPE_CHECKING
from typing import Any
from typing import Tuple

if TYPE_CHECKING:
    from jsonpath24 import ExpressionType


class FilterFunction(ABC):
    """Base class for JSONPath function extensions."""

    @property
    @abstractmethod
    def arg_types(self) -> Tuple[ExpressionType, ...]:
        """Argument types expected by the filter function."""

    @property
    @abstractmethod
    def return_type(self) -> ExpressionType:
        """The type of the value returned by the filter function."""

    @abstractmethod
    def __call__(self, *args: Any, **kwds: Any) -> Any:
        """Called the filter function."""
