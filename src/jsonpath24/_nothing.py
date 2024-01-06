"""The special value _Nothing_."""
class Nothing:
    def __eq__(self, other: object) -> bool:
        return isinstance(other, Nothing)

NOTHING = Nothing()

