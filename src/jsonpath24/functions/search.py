"""The standard `search` function extension."""

import re

from jsonpath24 import ExpressionType
from jsonpath24 import FilterFunction


class Search(FilterFunction):
    """A type-aware implementation of the standard `search` function."""

    arg_types = (ExpressionType.value, ExpressionType.value)
    return_type = ExpressionType.logical

    def __call__(self, string: str, pattern: str) -> bool:
        """Return `True` if _string_ contains _pattern_, or `False` otherwise."""
        try:
            # re.search caches compiled patterns internally
            return bool(re.search(pattern, string))
        except (TypeError, re.error):
            return False
