import gams.transfer as gt


def create_domain_example_1(file_path: str) -> None:
    m = gt.Container()

    i = gt.Set(m, 'i', records=['i1', 'i2'])
    a = gt.Variable(m, 'a', domain=[i])

    m.write(file_path)


def create_domain_example_2(file_path: str) -> None:
    m = gt.Container()

    i = gt.Set(m, 'i', records=['i2', 'i1'])
    a = gt.Variable(m, 'a', domain=[i])

    m.write(file_path)
