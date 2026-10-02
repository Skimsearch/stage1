package ulog;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


public final class IndexBuilder {

    private static final Pattern TOKEN_PATTERN = Pattern.compile("\\b[a-zA-Z]+\\b");

    public static final List<Integer> ALL_BOOKS = Arrays.asList(1342, 11, 84, 98, 1661, 2701);
    public static final int NEW_BOOK = 2701;

    private IndexBuilder() {}

    public static List<String> tokenize(String text) {
        List<String> tokens = new ArrayList<>();
        Matcher matcher = TOKEN_PATTERN.matcher(text.toLowerCase(Locale.ROOT));
        while (matcher.find()) {
            tokens.add(matcher.group());
        }
        return tokens;
    }

    public static Path resolveBodyPath(int bookId) {
        Path p = Paths.get("../../datalake/book/" + bookId + "/" + bookId + ".body.txt");
        if (!Files.exists(p)) {
            p = Paths.get("datalake/book/" + bookId + "/" + bookId + ".body.txt");
        }
        return p;
    }

    public static Map<String, List<Integer>> buildInvertedIndex(List<Integer> bookIds) throws IOException {
        Map<String, List<Integer>> index = new HashMap<>();

        for (int bookId : bookIds) {
            Path bodyPath = resolveBodyPath(bookId);
            if (!Files.exists(bodyPath)) {
                continue;
            }

            String text = Files.readString(bodyPath);
            Set<String> uniqueTokens = new HashSet<>(tokenize(text));

            for (String token : uniqueTokens) {
                index.computeIfAbsent(token, k -> new ArrayList<>()).add(bookId);
            }
        }

        return index;
    }

    public static Path resolveDatamartRoot() {
        Path p = Paths.get("../../datamart");
        if (!Files.exists(p)) {
            p = Paths.get("datamart");
        }
        return p.toAbsolutePath().normalize();
    }
}