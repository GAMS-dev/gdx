import gams.transfer as gt


def create_description_example_1(file_path: str) -> None:
    m = gt.Container()

    gt.Set(m, "i", records=[("seattle", "text 1"), ("san-diego", "text 2")])

    m.write(file_path)


def create_description_example_2(file_path: str) -> None:
    m = gt.Container()

    gt.Set(m, "i", records=[("seattle", "text 3"), ("san-diego", "text 4")])

    m.write(file_path)
