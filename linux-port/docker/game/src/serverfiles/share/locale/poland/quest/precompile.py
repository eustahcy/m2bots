TOKEN_LIST = [
	"-", "+", "*", "/",
	"<", ">", "!", "=", "~",
	"[", "]",
	"{", "}",
	"(", ")",
	"\t", "\n", "\r",
	" ", ",", ".",
]

def split_by_quat(buf):
	p = False
	l = list(buf)
	l.reverse()
	s = ""
	res = []
	while l:
		c = l.pop()
		if c == '"':
			if p == True:
				s += c
				res += [s]
				s = ""
			else:
				if len(s) != 0:
					res += [s]
				s = '"'
			p = not p
		elif c == "\\" and l[0] == '"':
			s += c
			s += l.pop()
		else:
			s += c

	if len(s) != 0:
		res += [s]

	return res

def AddSepMiddleOfElement(l, sep):
	l.reverse()
	new_list = [l.pop()]
	while l:
		new_list.append(sep)
		new_list.append(l.pop())
	return new_list

def my_split_with_seps(s, seps):
	res = [s]
	for sep in seps:
		new_res = []
		for r in res:
			sp = r.split(sep)
			sp = AddSepMiddleOfElement(sp, sep)
			new_res += sp
		res = new_res
	new_res = []
	for r in res:
		if r != "":
			new_res.append(r)
	return new_res

def my_split(s, seps):
	res = []
	curr_token = ""
	is_quoted = False

	for c in s:
		if c == '"':
			if is_quoted:
				curr_token += c
				res.append(curr_token)
				curr_token = ""
			else:
				if curr_token != "":
					res.append(curr_token)
				curr_token = '"'
			is_quoted = not is_quoted
		elif c in seps and not is_quoted:
			if curr_token != "":
				res.append(curr_token)
			curr_token = ""
		else:
			curr_token += c

	if curr_token != "":
		res.append(curr_token)

	return res

def MultiIndex(list, key):
	l = []
	i = 0
	for s in list:
		if s == key:
			l.append(i)
		i = i + 1
	return l

def Replace(lines, parameter_table, keys):
	r = []
	for string in lines:
		l = split_by_quat(string)
		for s in l:
			if s[0] == '"':
				r += [s]
			else:
				tokens = my_split_with_seps(s, TOKEN_LIST)
				for key in keys:
					try:
						indices = MultiIndex(tokens, key)
						for i in indices:
							if len(parameter_table[key]) > 1:
								if tokens[i + 1] == "[" and tokens[i + 3] == "]":
									tokens[i] = parameter_table[key][int(tokens[i + 2]) - 1]
									tokens[i + 1:i + 4] = ["", "", ""] # [x]
								else:
									tokens[i] = "{ " + ", ".join(str(x) for x in parameter_table[key]) + " }"
							else:
								tokens[i] = parameter_table[key][0]
					except:
						pass
				r += tokens
	return r

def MakeParameterTable(lines, parameter_table, keys):
	group_names = []
	group_values = []

	idx = 0
	for line in lines:
		idx += 1
		line = line.strip("\n")
		if (-1 != line.find("--")):
			line = line[0:line.find("--")]

		tokens = my_split(line, TOKEN_LIST)
		if len(tokens) == 0:
			continue

		if tokens[0] == "quest":
			start = idx
			break

		if tokens[0] == "define":
			if tokens[1] == "group":
				group_value = []
				for value in tokens[3:]:
					if parameter_table.get(value, 0) != 0:
						value = parameter_table[value]
					group_value.append(value)

				parameter_table[tokens[2]] = group_value
				keys.append(tokens[2])

			elif len(tokens) > 5:
				print("%d %s" % (idx, "Invalid syntax"))
				print("define <name> = <value>")
				print("define group <name> = [<val1>, <val2>, ...]")

			else:
				value = tokens[2]
				if parameter_table.get(value, 0) != 0:
					value = parameter_table[value]

				parameter_table[tokens[1]] = [value]
				keys.append(tokens[1])

	parameter_table = dict(zip(group_names, group_values))
	return start

def run(path, file_name):
	parameter_table = dict()
	keys = []

	path = path.strip("\n")
	if path == "":
		return False

	lines = open(path).readlines()
	start = MakeParameterTable(lines, parameter_table, keys)
	if len(keys) == 0:
		return False

	lines = lines[start - 1:]
	r = Replace(lines, parameter_table, keys)
	f = open("pre_qc/" + file_name, "w")
	for s in r:
		f.write(s)

	return True

if __name__ == "__main__":
	import sys
	if len(sys.argv) < 3:
		print("Usage: python pre_qc.py <input_file> <output_file>")
	else:
		run(sys.argv[1], sys.argv[2])