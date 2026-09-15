import os
import sys
import precompile

# sys.path.insert(1, './PRECOMPILE')
# import precompile

log = open("compile_log.txt",'w')

os.system('rmdir object /s /q')
log.write("Kasuje object\n")

# precompile.Run()

checkFolder = "quest"
compileList = []
print "### Start Quest Compilation ###"
for path, subdirs, files in os.walk(checkFolder):
	for name in files:
		if name.endswith('.quest') or name.endswith('.lua'):
			compileList.append(os.path.join(path, name))
			
for item in compileList:
	if "_unused" in item:
		continue
	for char in item:
		if char == "\\":
			item = item.replace("\\","/")
			
	baseName = os.path.basename(item)
	
	if not precompile.run(item, baseName):
		log.write("Compiling: " + item + "\n")
		os.system("qc.exe ./"+item)
	else:
		log.write("Compiling: pre_qc/" + baseName + "\n")
		os.system("qc.exe ./pre_qc/"+baseName)
		
	

log.close()
print "\nCOMPILATION SUCCESS! :-)"