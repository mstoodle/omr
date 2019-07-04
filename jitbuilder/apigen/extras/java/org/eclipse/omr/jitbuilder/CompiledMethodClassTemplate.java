package org.eclipse.omr.jitbuilder;

import java.lang.invoke.MethodHandles;

public class CompiledMethodClassTemplate {
	public static native void connect();

	public static MethodHandles.Lookup getLookup() { return MethodHandles.lookup(); }
}
