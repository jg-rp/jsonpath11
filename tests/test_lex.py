import jsonpath24


def assert_token_eq(
    t: jsonpath24.Token,
    type_: jsonpath24.TokenType,
    index: int,
    value: str,
    query: str,
) -> None:
    assert t.type == type_
    assert t.index == index
    assert t.value == value
    assert t.query == query


def test_tokenize() -> None:
    """Test that we can tokenize a JSONPath query."""
    query = "$.foo.bar"
    lexer = jsonpath24.Lexer(query)
    lexer.run()
    tokens = lexer.tokens()
    assert len(tokens) == 4  # + EOF  # noqa: PLR2004
    assert_token_eq(tokens[0], jsonpath24.TokenType.root, 0, "$", query)
    assert_token_eq(tokens[1], jsonpath24.TokenType.name_, 2, "foo", query)
    assert_token_eq(tokens[2], jsonpath24.TokenType.name_, 6, "bar", query)
    assert_token_eq(tokens[3], jsonpath24.TokenType.eof_, 9, "", query)
