from glob import glob

env = Environment()

env.Append(CCFLAGS='-g -pthread')
env.Append(CPPDEFINES=['NO_GDS', 'GDS_NOMULTITHREADING'])

sources = glob("*.c") + glob ("ai/*.c")

env.SharedLibrary('gamex86', source=sources, CPPPATH='/usr/include/mysql', LIBS='mysqlclient')
