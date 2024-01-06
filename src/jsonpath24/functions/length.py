"""The standard `length` function extension."""
from collections.abc import Sized
from typing import Union

from jsonpath24 import NOTHING
from jsonpath24 import Nothing
from jsonpath24 import ExpressionType
from jsonpath24 import FilterFunction


class Length(FilterFunction):
    """A type-aware implementation of the standard `length` function."""

    arg_types = (ExpressionType.value,)
    return_type = ExpressionType.value

    def __call__(self, obj: Sized) -> Union[int, Nothing]:
        """Return an object's length, or `None` if the object does not have a length."""
        try:
            return len(obj)
        except TypeError:
            return NOTHING
