import jsonpath24


def test_parse() -> None:
    """Test that we can parse a JSONPath query."""
    query = "$.foo.bar"
    segments = jsonpath24.parse(query)
    assert jsonpath24.to_string(segments) == "$['foo']['bar']"


def test_parse_recursive() -> None:
    """Test that we can parse a JSONPath query."""
    query = "$..[1]"
    segments = jsonpath24.parse(query)
    assert jsonpath24.to_string(segments) == "$..[1]"


def test_parse_issue() -> None:
    """Test that we can parse a JSONPath query."""
    query = "$[?@.a]"
    segments = jsonpath24.parse(query)
    assert jsonpath24.to_string(segments) == "$[?@['a']]"
    assert jsonpath24.to_string(segments) == "$[?@['a']]"
