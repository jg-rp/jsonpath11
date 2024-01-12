import jsonpath24
import pytest


def test_non_string_dict_key() -> None:
    """Test that we raise a TypeError when querying data with non-string keys."""
    query = "$[*]"
    data = {"1": "a", 2: "b"}
    with pytest.raises(TypeError, match="expected mapping with string keys, found 2"):
        jsonpath24.query(query, data)
