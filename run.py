#! /usr/bin/python3

import os, sys, csv
import datetime
import subprocess as sp
import multiprocessing as mp
import argparse as ap
import tempfile as tf

def getGGG(flag = ""):
	GGG = "GGG" + str(flag)
	print(GGG)
	return GGG

def initArgParser():
	parser = ap.ArgumentParser(description = "Run the experiments")
	parser.add_argument("method",
	                    type = str,
	                    help = "indicate the running method: " +
	                           "include " + ", ".join(sorted(methodToParam.keys())))
	parser.add_argument("-id", "--input-directory",
	                    type = str,
	                    default = "tests/hwmcc/",
	                    help = "indicate the directory containing the aig files",
	                    metavar = "inDir", dest = "inDir")
	parser.add_argument("-od", "--output-directory",
	                    type = str,
	                    default = "tests/log/",
	                    help = "indicate the directory containing the log files",
	                    metavar = "outDir", dest = "outDir")
	parser.add_argument("-t", "--timeout",
	                    type = int,
	                    default = 3600,
	                    help = "indicate the maximum runtime",
	                    metavar = "timeout")
	parser.add_argument("-p", "--process",
	                    type = int,
	                    default = 1,
	                    help = "indicate the maximum number of process",
	                    metavar = "process")
	parser.add_argument("-r", "--result",
	                    type = str,
	                    default = "result.csv",
	                    help = "indicate the filename of result",
	                    metavar = "result")
	parser.add_argument("-s", "--suffix",
	                    type = str,
	                    default = "aig",
	                    help = "indicate the suffix of filename",
	                    metavar = "suffix")
	parser.add_argument("-o", "--option",
	                    type = str,
	                    default = "",
	                    help = "indicate the options of the command",
	                    metavar = "option")
	parser.add_argument("-bc", "--before-command",
	                    type = str,
	                    default = "",
	                    help = "indicate the command before checking",
	                    metavar = "bCmd", dest = "bCmd")
	parser.add_argument("-ac", "--after-command",
	                    type = str,
	                    default = "",
	                    help = "indicate the command after checking",
	                    metavar = "aCmd", dest = "aCmd")
	return parser

class RunBase:
	def __init__(self, execFile, inputTemplate, checkFunc, inDir, outDir, timeout, option, bCmd, aCmd):
		self.execFile      = execFile
		self.inputTemplate = inputTemplate
		self.checkFunc     = checkFunc
		self.inDir         = inDir
		self.outDir        = outDir
		self.timeout       = timeout
		self.option        = option
		self.bCmd          = bCmd
		self.aCmd          = aCmd

	def __call__(self, caseFile):
		inFile  = tf.TemporaryFile(mode = "w+")
		outFile = open(self.outDir + caseFile + ".log", 'w+')

		inFile.write(self.inputTemplate.format(self.inDir + caseFile, self.bCmd, self.option, self.aCmd))
		inFile.seek(0)

		try:
			c = sp.run([self.execFile],
			           timeout = self.timeout,
			           stdin   = inFile,
			           stdout  = outFile,
			           stderr  = outFile)
			if c.returncode != 0:
				outFile.seek(0)
				print(); print(outFile.read()); print()
				return [caseFile, "ERROR"]
			outFile.seek(0)
			return [caseFile] + self.checkFunc(outFile.read())
		except sp.TimeoutExpired:
			return [caseFile, "TIMEOUT"]

def checkIa2bSimpS(log):
	line = log.split("\n")
	i = 0
	while i < len(line):
		if line[i] == "ia2b> test network -and":
			i += 1
			if i == len(line):
				return [getGGG()]
			try: return [int(line[i])]
			except ValueError: return [getGGG()]
		else:
			i += 1
	return [getGGG()]

def checkIa2bBalance(log):
	line = log.split("\n")
	level = []
	i = 0
	while i < len(line):
		if line[i] == "ia2b> print network -le":
			i += 2
			if i >= len(line):
				return [getGGG()]
			try:
				ii = 0
				while line[i][ii] != ",":
					ii += 1
				level.append(int(line[i][6:ii]))
			except ValueError: return [getGGG()]
		i += 1
	return level if len(level) == 2 else [getGGG()]

def checkAbcSimpS(log):
	line = log.split("\n")

def checkV3MC(log):
	line = log.split("\n")
	result = [None, None]
	for l in line:
		if l[:19] == "Inductive Invariant":
			if result[0] != None:
				print(line)
				return [getGGG(1)]
			result[0] = "UNSAT"
		elif l[:15] == "Counter-example":
			if result[0] != None:
				print(line)
				return [getGGG(2)]
			result[0] = "SAT"
		elif l[:18] == "Total time used  :":
			if result[1] != None:
				return [getGGG(3)]
				print(line)
			try: result[1] = "{:.2f}".format(float(l[19:-8]))
			except ValueError: print(line); return [getGGG(4)]
	if result[0] == None:
		print(line)
		return [getGGG(5)]
	if result[1] == None:
		print(line)
		return [getGGG(6)]
	return result

def checkAbcMC(log):
	line = log.split("\n")
	result = [None, None]
	for l in line:
		if l[:15] == "Property proved":
			if result[0] != None:
				print(line)
				return [getGGG(1)]
			result[0] = "UNSAT"
		elif "asserted" in l:
			if result[0] != None:
				print(line)
				return [getGGG(2)]
			result[0] = "SAT"
		elif l[:7] == "elapse:":
			if result[1] != None:
				print(line)
				return [getGGG(3)]
			i = 8
			while l[i] != " ":
				i += 1
			try: result[1] = "{:.2f}".format(float(l[8:i]))
			except ValueError: print(line); return [getGGG(4)]
		elif "combinational" in l:
			if result[0] != None:
				print(line)
				return [getGGG(5)]
			result[0] = "COMB"
		elif l[:15] == "Reached timeout":
			result[0] = "TIMEOUT"
	if result[0] == None:
		print(line)
		return [getGGG(6)]
	if result[1] == None:
		print(line)
		return [getGGG(7)]
	return result

def checkIa2bPdr(log):

	def printLine():
		print()
		for l in line:
			print(l)

	terSim = True
	Comb = False
	unsatGen = True
	prop = True
	line = log.split("\n")
	result = [None for i in range(12)]
	for l in line:
		if l[:15] == "Property proved":
			if result[0] != None:
				printLine()
				return [getGGG(1)]
			result[0] = "UNSAT"
		elif l[:25] == "Observe a counter example":
			if result[0] != None:
				printLine()
				return [getGGG(2)]
			result[0] = "SAT"
		elif l == "Cannot determinie the property":
			if result[0] != None:
				printLine()
				return [getGGG(2)]
			result[0] = "TIMEOUT"
		elif l[:21] == "Overall Total Time  :":
			if result[1] != None:
				printLine()
				return [getGGG(3)]
			try: result[1] = "{:.2f}".format(float(l[22:-8]))
			except ValurError: printLine(); return [getGGG(4)]
		elif l[:33] == "Number of ternary simulation   = ":
			if result[2] != None:
				printLine()
				return [getGGG(5)]
			try: result[2] = int(l[33:])
			except ValueError: printLine(); return [getGGG(6)]
		elif l[:33] == "Average literal count (TerSim) = ":
			if result[3] != None:
				printLine()
				return [getGGG(7)]
			try: result[3] = float(l[33:])
			except ValueError: printLine(); return [getGGG(8)]
		elif l[:33] == "Average removal count (TerSim) = ":
			if result[4] != None:
				printLine()
				return [getGGG(9)]
			try: result[4] = float(l[33:])
			except ValueError: printLine(); return [getGGG(10)]
		elif l[:33] == "Total runtime on TerSim        = ":
			if result[5] != None:
				printLine()
				return [getGGG(11)]
			try: result[5] = float(l[33:-2])
			except ValueError: printLine(); return [getGGG(12)]
		elif l[:33] == "Average runtime on TerSim      = ":
			if result[6] != None:
				printLine()
				return [getGGG(13)]
			try: result[6] = float(l[33:-2])
			except ValueError: printLine(); return [getGGG(14)]
		elif l[:20] == "Total runtime     = ":
			if result[7] != None:
				printLine()
				return [getGGG()]
			try: result[7] = float(l[20:-2])
			except ValueError: printLine(); return [getGGG()]
		elif l[:35] == "Runtime on remove stage          = ":
			if result[8] != None:
				printLine()
				return [getGGG()]
			try: result[8] = float(l[35:-2])
			except ValueError: printLine(); return [getGGG()]
		elif l[:35] == "Runtime on push stage            = ":
			if result[9] != None:
				printLine()
				return [getGGG()]
			try: result[9] = float(l[35:-2])
			except ValueError: printLine(); return [getGGG()]
		elif l[:35] == "Runtime on UNSAT Gen             = ":
			if result[10] != None:
				printLine()
				return [getGGG()]
			try: result[10] = float(l[35:-2])
			except ValueError: printLine(); return [getGGG()]
		elif l[:30] == "Runtime on cube propagation = ":
			if result[11] != None:
				printLine()
				return [getGGG()]
			try: result[11] = float(l[30:-2])
			except ValueError: printLine(); return [getGGG()]
		elif l == "No ternary simulation!":
			terSim = False
		elif l == "No latch related to the property. Reduce to combinational checker!":
			Comb = True
		elif l == "No UNSAT generalization!":
			unsatGen = False
		elif l == "No cube propagation!":
			prop = False
	if result[0] == None:
		printLine()
		return [getGGG(15)]
	if result[1] == None:
		printLine()
		return [getGGG(16)]
	if Comb:
		for i in range(3, 12):
			if result[i] != None:
				printLine()
				return [getGGG()]
		result[2] = "Comb"
		for i in range(3, 12):
			result[i] = ""
	else:
		if result[7] == None:
			printLine()
			return [getGGG()]
		if terSim:
			for i in range(2, 7):
				if result[i] == None:
					printLine()
					return [getGGG(i+17)]
		else:
			for i in range(2, 7):
				if result[i] != None:
					printLine()
					return [getGGG(i+17)]
			result[2] = "No TerSim"
			for i in range(3, 7):
				result[i] = ""
		if unsatGen:
			for i in range(8, 11):
				if result[i] == None:
					printLine()
					return [getGGG()]
		else:
			for i in range(8, 11):
				if result[i] != None:
					printLine()
					return [getGGG()]
			result[8] = "No UNSAT Gen"
			result[9] = ""
			result[10] = ""
		if prop:
			if result[11] == None:
				printLine()
				return [getGGG()]
		else:
			if result[11] != None:
				printLine()
				return [getGGG()]
			result[11] = "No Prop"
	return result

def checkIa2bPbc(log):

	def printLine():
		print()
		for l in line:
			print(l)

	line = log.split("\n")
	result = [None for i in range(2)]
	for l in line:
		if l[:15] == "Property proved":
			if result[0] != None:
				printLine()
				return [getGGG(1)]
			result[0] = "UNSAT"
		elif l[:25] == "Observe a counter example":
			if result[0] != None:
				printLine()
				return [getGGG(2)]
			result[0] = "SAT"
		elif l == "Cannot determinie the property":
			if result[0] != None:
				printLine()
				return [getGGG(2)]
			result[0] = "TIMEOUT"
		elif l[:21] == "Overall Total Time  :":
			if result[1] != None:
				printLine()
				return [getGGG(3)]
			try: result[1] = "{:.2f}".format(float(l[22:-8]))
			except ValurError: printLine(); return [getGGG(4)]

	if result[0] == None:
		printLine()
		return [getGGG(5)]
	if result[1] == None:
		printLine()
		return [getGGG(6)]
	return result

methodToParam = \
{
	"V3Pdr" : ["read aig {}\n"
	           "set report -all -off\n"
	           "set report -reset\n"
	           "set report -u -off\n"
	           "set safety 0\n"
	           "verify pdr p1 {} -m 50000\n"
	           "usage\n"
	           "q -f\n",
	           "../SoCV/hw5/B03901084_hw5/satv",
	           checkV3MC],
	"V3Itp" : ["read aig {}\n"
	           "set report -all -off\n"
	           "set report -reset\n"
	           "set report -u -off\n"
	           "set safety 0\n"
	           "verify itp p1 {} -m 50000\n"
	           "usage\n"
	           "q -f\n",
	           "../SoCV/hw5/B03901084_hw5/satv",
	           checkV3MC],
	"Ia2bSimpS" : ["read aig {}\n"
	               "simplify network -r\n"
	               "simplify network -s\n"
	               "simplify network -r\n"
	               "test network -and\n"
	               "quit -f\n",
	               "./ia2b",
	               checkIa2bSimpS],
	"AbcSimpS" : ["read_aiger {}\n"
	              "print_stats\n"
	              "quit",
	              "../LSV/hehe/abc",
	              checkAbcSimpS],
	"Ia2bBal" : ["read aig {}\n"
	             "print network -le\n"
	             "simplify network -b\n"
	             "print network -le\n"
	             "q -f\n",
	             "./ia2b",
	             checkIa2bBalance],
	"AbcPdr" : ["read_aiger {}\n"
	            "{}\n"
	            "pdr {}\n"
	            "{}\n"
	            "time\n"
	            "quit\n",
	            "../LSV/hehe/abc/abc_16",
	            checkAbcMC],
	"AbcItp" : ["read_aiger {}\n"
	            "int {}\n"
	            "time\n"
	            "quit\n",
	            "../LSV/hehe/abc/abc_16",
	            checkAbcMC],
	"Ia2bPdrRun" : ["read aig {}\n"
	                "{}\n"
	                "check safety pdr 0 {} -m 50000\n"
	                "{}\n"
	                "time\n"
	                "quit\n",
	                "./ia2b_run",
	                checkIa2bPdr],
	"Ia2bPdrTest" : ["read aig {}\n"
	                 "check safety pdr 0 {} -m 50000\n"
	                 "time\n"
	                 "quit\n",
	                 "./ia2b",
	                 checkIa2bPdr],
	"Ia2bPbcRun" : ["read aig {}\n"
	                "check safety pbc 0 {} -m 50000\n"
	                "time\n"
	                "quit\n",
	                "./ia2b_run",
	                checkIa2bPbc],
	"Ia2bPbcTest" : ["read aig {}\n"
	                 "check safety pbc 0 {} -m 50000\n"
	                 "time\n"
	                 "quit\n",
	                 "./ia2b",
	                 checkIa2bPbc]
}

if __name__ == "__main__":
	param = vars(initArgParser().parse_args())
	if param["process"] <= 0 or param["process"] > 10:
		print("Invalid process!"); sys.exit(1)
	try: runParam = methodToParam[param["method"]]
	except: print("No such method!"); sys.exit(1)
	runBase = RunBase(runParam[1], runParam[0], runParam[2],
	                  param["inDir"], param["outDir"], param["timeout"], param["option"], param["bCmd"], param["aCmd"])
	caseList = [file for file in os.listdir(param["inDir"]) if file.endswith(param["suffix"])]
#	runResult = mp.Pool(param["process"]).map(runBase, caseList)
	runResult = []

	print("=" * 36)
	print("Executable    =", runParam[1])
	print("Method        =", param["method"])
	print("Option        =", "None" if len(param["option"]) == 0 else param["option"])
	print("Pre Command   =", "None" if len(param["bCmd"]) == 0 else param["bCmd"])
	print("Post Command  =", "None" if len(param["aCmd"]) == 0 else param["aCmd"])
	print("Time Limit    =", param["timeout"])
	print("No. Core      =", param["process"])
	print("AIG direcotry =", param["inDir"])
	print("Log directory =", param["outDir"])
	print("Result File   =", param["result"])
	print("Suffix name   =", param["suffix"])
	print("=" * 36)
	print("Start  at:", datetime.datetime.now())
	sys.stdout.write("0/{}".format(len(caseList)))
	sys.stdout.flush()
	for i, result in enumerate(mp.Pool(param["process"]).imap_unordered(runBase, caseList), 1):
		sys.stdout.write("\r{}/{}".format(i, len(caseList)))
		sys.stdout.flush()
		runResult.append(result)
	print()
	print("Finish at:", datetime.datetime.now())

#	result = [[c] + r for c, r in zip(caseList, runResult)]
	runResult.sort(key = lambda k: k[0])
	with open(param["result"], 'w') as resultFile:
		csvWriter = csv.writer(resultFile)
		csvWriter.writerows(runResult)
