import proto as p
import examples as e

import timeit

p.solve(e.duck)
p.solve(e.swan)
p.settings['print_steps'] = True
p.settings['print_possibility_calcs'] = True
p.solve(e.snowflake)
