package cn.oureda.coralreefplayer.util;

import java.io.*;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.file.FileSystem;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.Map;

public class LibraryLoader {
    public static void extractFromJar(String src, Path dst) throws URISyntaxException, IOException {
        URL resource = LibraryLoader.class.getResource(src);
        FileSystem fileSystem = null;
        Path jarPath;
        try {
            fileSystem = FileSystems.newFileSystem(resource.toURI(), Map.of());
            jarPath = fileSystem.getPath("/");
        } catch (Exception e) {
            jarPath = Paths.get(resource.toURI());
        }

        Path jarPath2 = jarPath;
        Files.walkFileTree(jarPath, new SimpleFileVisitor<>() {
            private Path currentTarget;

            @Override
            public FileVisitResult preVisitDirectory(Path dir, BasicFileAttributes attrs) throws IOException {
                currentTarget = dst.resolve(jarPath2.relativize(dir).toString());
                Files.createDirectories(currentTarget);
                return FileVisitResult.CONTINUE;
            }

            @Override
            public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
                Files.copy(file, dst.resolve(jarPath2.relativize(file).toString()), StandardCopyOption.REPLACE_EXISTING);
                return FileVisitResult.CONTINUE;
            }
        });

        if (fileSystem != null)
            fileSystem.close();
    }

    public static void addPath(String path) {
        String pathVar;
        String os = System.getProperty("os.name").toLowerCase();
        if (os.contains("win")) {
            pathVar = "Path";
        } else if (os.contains("mac")) {
            pathVar = "DYLD_LIBRARY_PATH";
        } else {
            pathVar = "LD_LIBRARY_PATH";
        }
        String value = System.getenv(pathVar);
        value = value == null ? path : value + File.pathSeparator + path;
        System.setProperty(pathVar, value);
        System.setProperty("java.library.path", value);
    }

    @Deprecated
    public static void load(String libName) throws IOException {
        String fullLibName = System.mapLibraryName(libName);
        URL url = LibraryLoader.class.getResource("/native/" + fullLibName);
        Path tempDir = Path.of(System.getProperty("java.io.tmpdir"), "CoralReefPlayer");
        if (!Files.exists(tempDir))
            Files.createDirectory(tempDir);
        File file = tempDir.resolve(fullLibName).toFile();
        
        InputStream in = url.openStream();
        OutputStream out = new FileOutputStream(file);
        int length;
        byte[] buffer = new byte[8192];
        while ((length = in.read(buffer)) > 0)
            out.write(buffer, 0, length);
        in.close();
        out.close();

        file.deleteOnExit();
        System.load(file.getAbsolutePath());
    }

    public static boolean isWindows() {
        return System.getProperty("os.name").toLowerCase().contains("win");
    }

    public static boolean isAndroid() {
        try {
            Class.forName("android.os.Build");
            return true;
        } catch (ClassNotFoundException e) {
            return false;
        }
    }
}
