import gams.transfer as gt


def create_description_example_1(file_path: str) -> None:
    m = gt.Container()

    gt.Set(m, 'i', records=['seattle', 'san-diego'], description='example 1')

    m.write(file_path)


def create_description_example_2(file_path: str) -> None:
    m = gt.Container()

    gt.Set(m, 'i', records=['seattle', 'san-diego'], description='example 2')

    m.write(file_path)
