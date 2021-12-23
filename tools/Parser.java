
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.InputStreamReader;
import java.util.*;

public class Parser {
	
	private static double getLeakPower(String lpStr) {
		int index = lpStr.indexOf("(");
		String finalStr = lpStr.substring(index+1, lpStr.length());
		return Double.valueOf(finalStr);
	}
	
	static void process(String rootPath, String keyword, List<String> list) {
		try {
			String log = rootPath + "_" + keyword;
			BufferedReader reader = new BufferedReader(new FileReader(log));
			String line = reader.readLine();
			String[] words;
			List<String> wordList;
			String finalKeyword = "[" + keyword + "]";
			while (line != null) {
				words = line.split(":");
				wordList = Arrays.asList(words);
				if (wordList.contains(finalKeyword)) {
					String instance = words[2].replaceAll("\\s+","");
					list.add(instance);
				}
				line = reader.readLine();
			}
			reader.close();
		} catch (Exception e) {
			System.out.println(e);
		}
	}
	
	static void readTotal(String rootPath) {
		try {
			String actsimLog = rootPath + "_energy";
			BufferedReader reader = new BufferedReader(new FileReader(actsimLog));
			String line = reader.readLine();
			long totalEnergy = 0;
			long totalArea = 0;
			double totalLeak = 0;
			String[] words;
			List<String> wordList;
			while (line != null) {
				words = line.split("\\s+");
				wordList = Arrays.asList(words);
				if (wordList.contains("Total:")) {
					System.out.println(line);
					totalEnergy = Long.valueOf(words[1]);
					totalLeak = getLeakPower(words[2])*1e9;
					totalArea = Long.valueOf(words[5]);
				}
				line = reader.readLine();
			}
			System.out.printf("totalLeak: %5.1f, totalArea: %d\n", totalLeak, totalArea);
			reader.close();
		} catch (Exception e) {
			System.out.println(e);
		}
	}
	
	static void collectMetrics(String rootPath, List<String> mergeList, List<String> splitList, List<String> memList, List<String> fuList, List<String> actnCpList, List<String> actnDpList) {
		try {
			String actsimLog = rootPath + "_energy";
			BufferedReader reader = new BufferedReader(new FileReader(actsimLog));
			String line = reader.readLine();
			long mergeEnergy = 0;
			long splitEnergy = 0;
			long fuEnergy = 0;
			long actnCpEnergy = 0;
			long actnDpEnergy = 0;
			long memEnergy = 0;
			long mergeArea = 0;
			long splitArea = 0;
			long fuArea = 0;
			long memArea = 0;
			long actnCpArea = 0;
			long actnDpArea = 0;
			double mergeLP = 0;
			double splitLP = 0;
			double memLP = 0;
			double fuLP = 0;
			double actnCpLP = 0;
			double actnDpLP = 0;
			String[] words;
			List<String> wordList;
			while (line != null) {
				words = line.split("\\s+");
				line = reader.readLine();
				wordList = Arrays.asList(words);
				if (wordList.contains("---:subtree")) {
					continue;
				} else if (wordList.contains("Total:")) {
					continue;
				}
				String opName = words[2].replaceAll("\\s+","");
				String[] ops = opName.split("\\.");
				String finalOp = ops[ops.length - 1];
				long energy = Long.valueOf(words[3]);
				long area = Long.valueOf(words[7]);
				double lp = getLeakPower(words[4])*1e9;
				if (mergeList.contains(finalOp)) {
					mergeEnergy += energy;
					mergeArea += area;
					mergeLP += lp;
				} else if (splitList.contains(finalOp)) {
					splitEnergy += energy;
					splitArea += area;
					splitLP += lp;
				} else if (memList.contains(finalOp)) {
					memEnergy += energy;
					memArea += area;
					memLP += lp;
				} else if (fuList.contains(finalOp)) {
					fuEnergy += energy;
					fuArea += area;
					fuLP += lp;
				}
				if (actnCpList.contains(finalOp)) {
					actnCpEnergy += energy;
					actnCpArea += area;
					actnCpLP += lp;
				} else if (actnDpList.contains(finalOp)) {
					actnDpEnergy += energy;
					actnDpArea += area;
					actnDpLP += lp;
				}
			}
			System.out.println("\nENERGY: (mergeEnergy splitEnergy memEnergy fuEnergy actnCpEnergy actnDpEnergy)");
			System.out.printf("%d %d %d %d %d %d\n", mergeEnergy, splitEnergy, memEnergy, fuEnergy, actnCpEnergy, actnDpEnergy);
			System.out.println("\nAREA: (mergeArea splitArea memArea fuArea actnCpArea actnDpArea)");
			System.out.printf("%d %d %d %d %d %d\n", mergeArea, splitArea, memArea, fuArea, actnCpArea, actnDpArea);
			System.out.println("\nLeakPower: (mergeLP splitLP memLP fuLP actnCpLP actnDpLP)");
			System.out.printf("%.0f %.0f %.0f %.0f %.0f %.0f\n", mergeLP, splitLP, memLP, fuLP, actnCpLP, actnDpLP);
			reader.close();
		} catch (Exception e) {
			System.out.println(e);
		}
	}
	
	public static void main(String[] args) {
		String rootPath = args[0];
		int[] statistics = new int[16];
		try {
			List<String> mergeList = new ArrayList<String>();
			List<String> splitList = new ArrayList<String>();
			List<String> memList = new ArrayList<String>();
			List<String> fuList = new ArrayList<String>();
			List<String> actnCpList = new ArrayList<String>();
			List<String> actnDpList = new ArrayList<String>();
			process(rootPath, "merge", mergeList);
			process(rootPath, "split", splitList);
			process(rootPath, "mem", memList);
			process(rootPath, "fu", fuList);
			process(rootPath, "actnCp", actnCpList);
			process(rootPath, "actnDp", actnDpList);
			readTotal(rootPath);
			collectMetrics(rootPath, mergeList, splitList, memList, fuList, actnCpList, actnDpList);
		} catch (Exception e) {
			System.out.println(e);
		}
	}
}