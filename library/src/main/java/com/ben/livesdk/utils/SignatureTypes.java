package com.ben.livesdk.utils;

import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Type;

/**
 * @author @zhangchuan622@gmail.com
 * @version 1.0
 * @create 2019/8/1
 */
public class SignatureTypes {

    public static Method getMethodByJavaClassNameInNative(String className,String methodName){
        if (className==null || methodName==null) return null;
        try {
            return Class.forName(className.replaceAll("/", ".")).getDeclaredMethod(methodName);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public static boolean isAndroid(final String vmName) {
        final String lowerVMName = vmName.toLowerCase();
        return lowerVMName.contains("dalvik") || lowerVMName.contains("lemur");
    }

    public static boolean isAndroid() {
        return isAndroid(System.getProperty("java.vm.name"));
    }

    public static String getSignature(final Method method) {
        final StringBuffer buf = new StringBuffer();
        buf.append("(");
        final Class<?>[] types = method.getParameterTypes();
        for (int i = 0; i < types.length; ++i) {
            buf.append(getSignature(types[i]));
        }
        buf.append(")");
        buf.append(getSignature(method.getReturnType()));
        return buf.toString();
    }

    public static String getSignature(final Class<?> returnType) {
        if (returnType.isPrimitive()) {
            return getPrimitiveLetter(returnType);
        }
        if (returnType.isArray()) {
            return "[" + getSignature(returnType.getComponentType());
        }
        return "L" + getType(returnType) + ";";
    }

    public static String getType(final Class<?> parameterType) {
        if (parameterType.isArray()) {
            return "[" + getSignature(parameterType.getComponentType());
        }
        if (!parameterType.isPrimitive()) {
            final String clsName = parameterType.getName();
            return clsName.replaceAll("\\.", "/");
        }
        return getPrimitiveLetter(parameterType);
    }

    public static String getPrimitiveLetter(final Class<?> type) {
        if (Integer.TYPE.equals(type)) {
            return "I";
        }
        if (Void.TYPE.equals(type)) {
            return "V";
        }
        if (Boolean.TYPE.equals(type)) {
            return "Z";
        }
        if (Character.TYPE.equals(type)) {
            return "C";
        }
        if (Byte.TYPE.equals(type)) {
            return "B";
        }
        if (Short.TYPE.equals(type)) {
            return "S";
        }
        if (Float.TYPE.equals(type)) {
            return "F";
        }
        if (Long.TYPE.equals(type)) {
            return "J";
        }
        if (Double.TYPE.equals(type)) {
            return "D";
        }
        throw new IllegalStateException("Type: " + type.getCanonicalName() + " is not a primitive type");
    }

    public static Type getMethodType(final Class<?> clazz, final String methodName) {
        try {
            final Method method = clazz.getMethod(methodName, (Class<?>[]) new Class[0]);
            return method.getGenericReturnType();
        } catch (Exception ex) {
            return null;
        }
    }

    public static Type getFieldType(final Class<?> clazz, final String fieldName) {
        try {
            final Field field = clazz.getField(fieldName);
            return field.getGenericType();
        } catch (Exception ex) {
            return null;
        }
    }

}
