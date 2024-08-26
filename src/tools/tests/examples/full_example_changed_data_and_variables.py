import gams.transfer as gt
import pandas as pd


def create_full_example_changed_data_and_variables(file_path: str) -> None:
    # create an empty Container object
    m = gt.Container()

    # add sets
    i = gt.Set(m, 'i', records=['seattle', 'san-diego'], description='supply')
    j = gt.Set(m, 'j', records=['new-york', 'chicago', 'topeka'], description='markets')

    # add parameters
    a = gt.Parameter(m, 'a', ['*'], description='capacity of plant i in cases')
    b = gt.Parameter(m, 'b', [j], description='demand at market j in cases')
    d = gt.Parameter(m, 'd', [i, j], description='distance in thousands of miles')
    f = gt.Parameter(
        m, 'f', records=90, description='freight in dollars per case per thousand miles'
    )
    c = gt.Parameter(
        m, 'c', [i, j], description='transport cost in thousands of dollars per case'
    )

    # set parameter records
    cap = pd.DataFrame([('seattle', 350), ('san-diego', 600)], columns=['plant', 'n_cases'])
    a.setRecords(cap)

    dem = pd.DataFrame(
        [('new-york', 325), ('chicago', 300), ('topeka', 275)],
        columns=['market', 'n_cases'],
    )
    b.setRecords(dem)

    dist = pd.DataFrame(
        [
            ('seattle', 'new-york', 3.5),
            ('seattle', 'chicago', 2.7),
            ('seattle', 'topeka', 2.8),
            ('san-diego', 'new-york', 3.5),
            ('san-diego', 'chicago', 2.8),
            ('san-diego', 'topeka', 2.4),
        ],
        columns=['from', 'to', 'thousand_miles'],
    )
    d.setRecords(dist)

    # c(i,j) = f * d(i,j) / 1000;
    cost = d.records.copy(deep=True)
    cost['value'] = f.records.loc[0, 'value'] * cost['value'] / 1000
    c.setRecords(cost)

    # add variables
    q = pd.DataFrame(
        [
            ('seattle', 'new-york', 150, 0),
            ('seattle', 'chicago', 400, 0),
            ('seattle', 'topeka', 0, 0.036),
            ('san-diego', 'new-york', 375, 0),
            ('san-diego', 'chicago', 0, 0.009),
            ('san-diego', 'topeka', 375, 0),
        ],
        columns=['from', 'to', 'level', 'marginal'],
    )
    x = gt.Variable(
        m, 'x', 'positive', [i, j], records=q, description='shipment quantities in cases',
    )
    z = gt.Variable(
        m,
        'z',
        records=pd.DataFrame(data=[153.675], columns=['level']),
        description='total transportation costs in thousands of dollars',
    )

    # add equations
    cost = gt.Equation(m, 'cost', 'eq', description='define objective function')
    supply = gt.Equation(m, 'supply', 'leq', [i], description='observe supply limit at plant i')
    demand = gt.Equation(m, 'demand', 'geq', [j], description='satisfy demand at market j')

    # set equation records
    cost.setRecords(
        pd.DataFrame(data=[[0, 1, 0, 0]], columns=['level', 'marginal', 'lower', 'upper'])
    )

    supplies = pd.DataFrame(
        [
            ('seattle', 350, 'eps', float('-inf'), 350),
            ('san-diego', 550, 0, float('-inf'), 600),
        ],
        columns=['from', 'level', 'marginal', 'lower', 'upper'],
    )
    supply.setRecords(supplies)

    demands = pd.DataFrame(
        [
            ('new-york', 325, 0.225, 325),
            ('chicago', 300, 0.153, 300),
            ('topeka', 275, 0.126, 275),
        ],
        columns=['from', 'level', 'marginal', 'lower'],
    )
    demand.setRecords(demands)

    m.write(file_path)
