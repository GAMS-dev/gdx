import gams.transfer as gt


def create_order_example_1(file_path: str) -> None:
    m = gt.Container()

    gt.Set(m, "t1", records=[1987, 1988, 1989, 1990, 1991])
    gt.Set(m, "t2", records=[1983, 1984, 1985, 1986, 1987])
    gt.Set(m, "t3", records=[1987, 1989, 1991, 1983, 1985])

    m.write(file_path)


def create_order_example_2(file_path: str) -> None:
    m = gt.Container()

    gt.Set(m, "t2", records=[1987, 1988, 1989, 1990, 1991])
    gt.Set(m, "t3", records=[1983, 1984, 1985, 1986, 1987])
    gt.Set(m, "t1", records=[1987, 1989, 1991, 1983, 1985])

    m.write(file_path)
