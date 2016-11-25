import solver as s
import examples as e

import timeit

s.solve(e.duck)
s.solve(e.swan)
s.settings['print_steps'] = True
s.settings['print_possibility_calcs'] = True
s.solve(e.snowflake)
