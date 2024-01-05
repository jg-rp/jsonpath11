import jsonpath24


def test_singular_query() -> None:
    """Test that we can identify singular queries."""
    query = "$.foo"
    segments = jsonpath24.parse(query)
    assert jsonpath24.singular_query(segments)


def test_non_singular_query() -> None:
    """Test that we can identify non-singular queries."""
    query = "$.*"
    segments = jsonpath24.parse(query)
    assert not jsonpath24.singular_query(segments)
