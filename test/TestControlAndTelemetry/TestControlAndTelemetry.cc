/**
 * @file
 * TestControlAndTelemetry interface and implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <Vuo/Vuo.h>
#include <sstream>


// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(list<string>);
Q_DECLARE_METATYPE(vector<VuoRunner::Port *>);


/**
 * Tests the execution of compositions.
 */
class TestControlAndTelemetry : public TestCompositionExecution
{
	Q_OBJECT

private:

	/**
	 * Builds the composition at @c compositionPath into an executable and returns a newly allocated
	 * @c VuoRunner for that executable.
	 */
	static VuoRunner * createRunnerInNewProcess(const string &compositionPath, VuoCompilerBitcodeGenerator **generator = NULL)
	{
		VuoCompiler *compiler = initCompiler();

		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string exePath = VuoFileUtilities::makeTmpFile(file, "");

		if (generator)
		{
			*generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(compositionPath, compiler);
			compiler->compileComposition(*generator, bcPath);
		}
		else
		{
			compiler->compileComposition(compositionPath, bcPath);
		}
		compiler->linkCompositionToCreateExecutable(bcPath, exePath);
		delete compiler;
		remove(bcPath.c_str());

		VuoRunner * runner = VuoRunner::newSeparateProcessRunnerFromExecutable(exePath, true);
		runner->setDelegate(new TestRunnerDelegate());
		return runner;
	}

	/**
	 * Builds the composition at @c compositionPath into a dylib and returns a newly allocated
	 * @c VuoRunner for that dylib.
	 */
	static VuoRunner * createRunnerInNewProcessWithDylib(const string &compositionPath)
	{
		VuoCompiler *compiler = initCompiler();

		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource", "dylib");

		compiler->compileComposition(compositionPath, bcPath);
		vector<string> alreadyLinkedResourcePaths;
		set<string> alreadyLinkedResources;
		compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
		string compositionLoaderPath = compiler->getCompositionLoaderPath();
		delete compiler;
		remove(bcPath.c_str());

		VuoRunner * runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compositionLoaderPath, dylibPath, resourceDylibPath, true);
		// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
		return runner;
	}

	/**
	 * Builds the composition at @c compositionPath into a dylib and returns a newly allocated
	 * @c VuoRunner for that dylib.
	 */
	static VuoRunner * createRunnerInCurrentProcess(const string &compositionPath)
	{
		VuoCompiler *compiler = initCompiler();

		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		compiler->compileComposition(compositionPath, bcPath);
		compiler->linkCompositionToCreateDynamicLibrary(bcPath, dylibPath);
		delete compiler;
		remove(bcPath.c_str());

		VuoRunner * runner = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(dylibPath, true);
		// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
		return runner;
	}

	/**
	 * Helper class for running WriteTimesToFile.vuo.
	 */
	class WriteTimesToFileHelper
	{
	private:
		string path;

	public:
		void deleteFile(void) const
		{
			remove(path.c_str());
			QVERIFY(! VuoFileUtilities::fileExists(path));
		}

		WriteTimesToFileHelper(void)
		{
			path = VuoFileUtilities::makeTmpFile("WriteTimesToFile", "txt");
			setenv("TESTCONTROLANDTELEMETRY_WRITETIMESTOFILE", path.c_str(), 1);  // Pass to vuo.test.writeTimeToFile without using a published input port
			deleteFile();
		}

		~WriteTimesToFileHelper(void)
		{
			deleteFile();
		}

		/**
		 * Waits until a file exists at the path.
		 */
		void waitUntilFileExists(void) const
		{
			bool isExistent = false;
			int MAX_NUM_TRIES = 200;
			for (int i = 0; i < MAX_NUM_TRIES; ++i)
			{
				if (VuoFileUtilities::fileExists(path))
				{
					isExistent = true;
					break;
				}
				usleep(USEC_PER_SEC / 100);
			}
			QVERIFY(isExistent);
		}

		/**
		 * Checks that the file contains at least one timestamp, and all timestamps are between the two given timestamps.
		 */
		void checkTimesInFile(double beforeTime, double afterTime) const
		{
			ifstream fin(path.c_str());
			bool isEmpty = true;
			double readTime;
			while (fin >> readTime)
			{
				ostringstream sout;
				sout << std::fixed;  // not scientific notation
				sout << beforeTime << " < " << readTime << " < " << afterTime;
				QVERIFY2(beforeTime < readTime && readTime < afterTime, sout.str().c_str());
				isEmpty = false;
			}
			QVERIFY(! isEmpty);
			fin.close();
		}
	};

private slots:

	void testStartingAndStoppingComposition()
	{
		string compositionPath = getCompositionPath("WriteTimesToFile.vuo");

		{
			printf("    New process, executable\n");

			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			double beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();
			helper.waitUntilFileExists();
			runner->stop();
			double afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			delete runner;

			helper.checkTimesInFile(beforeStartTime, afterStopTime);
		}

		{
			printf("    New process, dylib\n");

			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcessWithDylib(compositionPath);
			double beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();
			helper.waitUntilFileExists();
			runner->stop();
			double afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			delete runner;

			helper.checkTimesInFile(beforeStartTime, afterStopTime);
		}

		{
			printf("    Current process, runOnMainThread()\n");

			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			double beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   helper->waitUntilFileExists();
							   runner->stop();
						   });
			runner->runOnMainThread();
			double afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			delete runner;

			helper->checkTimesInFile(beforeStartTime, afterStopTime);
			delete helper;
		}

		{
			printf("    Current process, drainMainDispatchQueue()\n");

			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			double beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   helper->waitUntilFileExists();
							   runner->stop();
			});
			while (! runner->isStopped())
			{
				runner->drainMainDispatchQueue();
				usleep(USEC_PER_SEC / 1000);
			}
			double afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			delete runner;

			helper->checkTimesInFile(beforeStartTime, afterStopTime);
			delete helper;
		}

		printf("    Error handling\n");

		{
			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();
			try
			{
				runner->runOnMainThread();
				QFAIL("Exception not thrown for runOnMainThread() with new process.");
			}
			catch (std::logic_error) { }
			runner->stop();
			delete runner;
		}

		{
			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			runner->start();
			dispatch_group_t group = dispatch_group_create();
			dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
			dispatch_group_async(group, queue, ^{
									 try
									 {
										 runner->runOnMainThread();
										 QFAIL("Exception not thrown for runOnMainThread() run on non-main thread.");
									 }
									 catch (std::logic_error) { }
								 });
			dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
			dispatch_async(queue, ^{
							   runner->stop();
						   });
			runner->runOnMainThread();
			delete runner;
		}
	}

	void testRunningMultipleCompositionsSimultaneously()
	{
		string compositionPath = getCompositionPath("Recur_Count.vuo");

		VuoRunner *runner1 = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath);
		runner1->start();

		VuoRunner *runner2 = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath);
		runner2->start();

		runner2->stop();
		runner1->stop();
	}

	void testPausingAndUnpausingComposition()
	{
		string compositionPath = getCompositionPath("WriteTimesToFile.vuo");
		const int PAUSE_CHECK_USEC = USEC_PER_SEC;

		{
			printf("    New process, started paused\n");

			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->startPaused();
			usleep(PAUSE_CHECK_USEC);

			for (int i = 0; i < 2; ++i)
			{
				printf("        iteration %d\n", i);
				double beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				runner->unpause();

				helper.waitUntilFileExists();
				runner->pause();
				double afterPauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				usleep(PAUSE_CHECK_USEC);

				helper.checkTimesInFile(beforeUnpauseTime, afterPauseTime);
				helper.deleteFile();
			}

			runner->stop();
			delete runner;
		}

		{
			printf("    New process, started unpaused\n");

			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			double beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();

			for (int i = 0; i < 2; ++i)
			{
				printf("        iteration %d\n", i);
				if (i > 0)
				{
					beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
					runner->unpause();
				}

				helper.waitUntilFileExists();
				runner->pause();
				double afterPauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				usleep(PAUSE_CHECK_USEC);

				helper.checkTimesInFile(beforeUnpauseTime, afterPauseTime);
				helper.deleteFile();
			}

			runner->stop();
			delete runner;
		}

		{
			printf("    Current process, started paused\n");

			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			runner->startPaused();
			usleep(PAUSE_CHECK_USEC);

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   for (int i = 0; i < 2; ++i)
							   {
								   printf("        iteration %d\n", i);
								   double beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
								   runner->unpause();

								   helper->waitUntilFileExists();
								   runner->pause();
								   double afterPauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
								   usleep(PAUSE_CHECK_USEC);

								   helper->checkTimesInFile(beforeUnpauseTime, afterPauseTime);
								   helper->deleteFile();
							   }
							   runner->stop();
						   });

			runner->runOnMainThread();
			delete runner;
			delete helper;
		}

		{
			printf("    Current process, started unpaused\n");

			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			__block double beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   for (int i = 0; i < 2; ++i)
							   {
								   printf("        iteration %d\n", i);
								   if (i > 0)
								   {
									   beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
									   runner->unpause();
								   }

								   helper->waitUntilFileExists();
								   runner->pause();
								   double afterPauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
								   usleep(PAUSE_CHECK_USEC);

								   helper->checkTimesInFile(beforeUnpauseTime, afterPauseTime);
								   helper->deleteFile();
							   }
							   runner->stop();
						   });

			runner->runOnMainThread();
			delete runner;
			delete helper;
		}
	}

private:

	class TestGettingPortValuesRunnerDelegate : public TestRunnerDelegate
	{
	private:
		struct IdentifierAndValue
		{
			QString identifier;
			QString value;
		};

		VuoRunner *runner;
		bool isStopping;
		int firstEventCountSeen;
		string firedPortIdentifier;
		vector<IdentifierAndValue> actualIdentifiersAndValues;

	public:
		TestGettingPortValuesRunnerDelegate()
		{
			runner = NULL;
			isStopping = false;
			firstEventCountSeen = 0;
		}

		~TestGettingPortValuesRunnerDelegate()
		{
			delete runner;
		}

		void runComposition()
		{
			string compositionPath = TestCompositionExecution::getCompositionPath("Recur_Count_Add.vuo");
			VuoCompilerBitcodeGenerator *generator = NULL;
			runner = createRunnerInNewProcess(compositionPath, &generator);

			string incrementPortIdentifier;
			string countPortIdentifier;
			string item1PortIdentifier;
			string listPortIdentifier;
			string termsPortIdentifier;
			string sumPortIdentifier;
			foreach (VuoNode *node, generator->composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->getClassName() == "vuo.test.firePeriodicallyWithCount")
				{
					{
						VuoPort *basePort = node->getOutputPortWithName("fired");
						firedPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
				}
				else if (node->getNodeClass()->getClassName() == "vuo.math.count.integer")
				{
					{
						VuoPort *basePort = node->getInputPortWithName("increment");
						incrementPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
					{
						VuoPort *basePort = node->getOutputPortWithName("count");
						countPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
				}
				else if (node->getNodeClass()->getClassName() == "vuo.list.make.2.VuoInteger")
				{
					{
						VuoPort *basePort = node->getInputPortWithName("item1");
						item1PortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
					{
						VuoPort *basePort = node->getOutputPortWithName("list");
						listPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
				}
				else if (node->getNodeClass()->getClassName() == "vuo.math.add.integer")
				{
					{
						VuoPort *basePort = node->getInputPortWithName("terms");
						termsPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
					{
						VuoPort *basePort = node->getOutputPortWithName("sum");
						sumPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
				}
			}

			delete generator;

			runner->setDelegate(this);

			runner->start();
			runner->waitUntilStopped();

			vector<IdentifierAndValue> expectedIdentifiersAndValues;
			int count = 0;
			for (int eventCount = 1; expectedIdentifiersAndValues.size() < actualIdentifiersAndValues.size(); ++eventCount)
			{
				count += eventCount;
				if (eventCount >= firstEventCountSeen)
				{
					IdentifierAndValue firedPair = { firedPortIdentifier.c_str(), QString("%1").arg(eventCount) };
					expectedIdentifiersAndValues.push_back(firedPair);
					IdentifierAndValue incrementPair = { incrementPortIdentifier.c_str(), QString("%1").arg(eventCount) };
					expectedIdentifiersAndValues.push_back(incrementPair);
					IdentifierAndValue countPair = { countPortIdentifier.c_str(), QString("%1").arg(count) };
					expectedIdentifiersAndValues.push_back(countPair);
					IdentifierAndValue item1Pair = { item1PortIdentifier.c_str(), QString("%1").arg(count) };
					expectedIdentifiersAndValues.push_back(item1Pair);
					IdentifierAndValue listPair = { listPortIdentifier.c_str(), QString("List containing elements: <ul><li>%1</li><li>10</li></ul>").arg(count) };
					expectedIdentifiersAndValues.push_back(listPair);
					IdentifierAndValue termsPair = { termsPortIdentifier.c_str(), QString("List containing elements: <ul><li>%1</li><li>10</li></ul>").arg(count) };
					expectedIdentifiersAndValues.push_back(termsPair);
					IdentifierAndValue sumPair = { sumPortIdentifier.c_str(), QString("%1").arg(count + 10) };
					expectedIdentifiersAndValues.push_back(sumPair);
				}
			}

			for (int i = 0; i < expectedIdentifiersAndValues.size(); ++i)
			{
				QCOMPARE(actualIdentifiersAndValues[i].identifier + " " + actualIdentifiersAndValues[i].value,
						 expectedIdentifiersAndValues[i].identifier + " " + expectedIdentifiersAndValues[i].value);
			}
		}

		void appendIdentifierAndValue(string portIdentifier, string valueAsString)
		{
			if (isStopping)
				return;

			// The composition may have started counting up before the runner was able to connect and start receiving telemetry.
			if (actualIdentifiersAndValues.empty())
			{
				if (portIdentifier != firedPortIdentifier)
					return;

				firstEventCountSeen = atoi(valueAsString.c_str());
			}

			IdentifierAndValue identifierAndValue = { portIdentifier.c_str(), valueAsString.c_str() };
			actualIdentifiersAndValues.push_back(identifierAndValue);

			if (actualIdentifiersAndValues.size() == 70)
			{
				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
				isStopping = true;
			}
		}

		void receivedTelemetryInputPortUpdated(string portIdentifier, bool receivedEvent, bool receivedData, string dataSummary)
		{
			appendIdentifierAndValue(portIdentifier, dataSummary);
		}

		void receivedTelemetryOutputPortUpdated(string portIdentifier, bool sentData, string dataSummary)
		{
			if (sentData)  // ignore done port
				appendIdentifierAndValue(portIdentifier, dataSummary);
		}
	};

private slots:

	void testGettingPortValues()
	{
		TestGettingPortValuesRunnerDelegate delegate;
		delegate.runComposition();
	}

private:

	class TestSettingAndGettingPortValuesRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		int timesCountSentEvent;
		int timesIncrementReceivedData;
		int timesIncrementReceivedEvent;
		VuoInteger firstCountSeen;
		string incrementPortIdentifier;
		string decrementPortIdentifier;
		string countPortIdentifier;
		VuoInteger incrementPortValue;
		bool isStopping;

	public:
		TestSettingAndGettingPortValuesRunnerDelegate()
		{
			runner = NULL;
			timesCountSentEvent = 0;
			timesIncrementReceivedData = 0;
			timesIncrementReceivedEvent = 0;
			isStopping = false;
		}

		~TestSettingAndGettingPortValuesRunnerDelegate()
		{
			delete runner;
		}

		void runComposition()
		{
			string compositionPath = TestCompositionExecution::getCompositionPath("Recur_Count.vuo");
			VuoCompilerBitcodeGenerator *generator = NULL;
			runner = createRunnerInNewProcess(compositionPath, &generator);

			foreach (VuoNode *node, generator->composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->getClassName() == "vuo.math.count.integer")
				{
					{
						VuoPort *basePort = node->getInputPortWithName("increment");
						VuoCompilerInputEventPort *compilerPort = static_cast<VuoCompilerInputEventPort *>(basePort->getCompiler());
						incrementPortIdentifier = compilerPort->getIdentifier();
					}
					{
						VuoPort *basePort = node->getInputPortWithName("decrement");
						VuoCompilerInputEventPort *compilerPort = static_cast<VuoCompilerInputEventPort *>(basePort->getCompiler());
						decrementPortIdentifier = compilerPort->getIdentifier();
					}
					{
						VuoPort *basePort = node->getOutputPortWithName("count");
						VuoCompilerOutputEventPort *compilerPort = static_cast<VuoCompilerOutputEventPort *>(basePort->getCompiler());
						countPortIdentifier = compilerPort->getIdentifier();
					}
				}
			}

			delete generator;

			incrementPortValue = 2;

			runner->setDelegate(this);

			runner->start();
			runner->waitUntilStopped();

			QCOMPARE(timesIncrementReceivedEvent, 4);
			QCOMPARE(timesIncrementReceivedData, 2);
		}

		void receivedTelemetryInputPortUpdated(string portIdentifier, bool receivedEvent, bool receivedData, string dataSummary)
		{
			if (portIdentifier != incrementPortIdentifier)
				return;

			if (receivedEvent)
				++timesIncrementReceivedEvent;

			if (receivedData)
				++timesIncrementReceivedData;

			QCOMPARE((VuoInteger)atol(dataSummary.c_str()), incrementPortValue);
		}

		void receivedTelemetryOutputPortUpdated(string portIdentifier, bool sentData, string dataSummary)
		{
			if (isStopping)
				return;

			if (portIdentifier != countPortIdentifier)
				return;

			runner->pause();

			QCOMPARE(sentData, true);

			// For this value to be in sync with valueAsString, the composition needs to have been paused before the next event fired.
			// The firing rate of the composition is set slow enough that the composition is very likely to be paused in time.
			VuoInteger countFromRunner = VuoInteger_valueFromJson( runner->getOutputPortValue(countPortIdentifier) );

			VuoInteger incrementFromRunner = VuoInteger_valueFromJson( runner->getInputPortValue(incrementPortIdentifier) );
			VuoInteger countFromSummary = atol(dataSummary.c_str());

			if (timesCountSentEvent == 0)
			{
				// The composition may have started counting up before the runner was able to connect and start receiving telemetry.
				firstCountSeen = countFromSummary;
			}
			else if (timesCountSentEvent == 1)
			{
				QCOMPARE(incrementFromRunner, (VuoInteger)2);

				VuoInteger expectedCount = firstCountSeen + 1;
				QCOMPARE(countFromSummary, expectedCount);
				QCOMPARE(countFromRunner, expectedCount);

				incrementPortValue = 100;
				runner->setInputPortValue(incrementPortIdentifier, VuoInteger_jsonFromValue(incrementPortValue));
				runner->setInputPortValue(decrementPortIdentifier, VuoInteger_jsonFromValue(2));
			}
			else if (timesCountSentEvent == 2)
			{
				QCOMPARE(incrementFromRunner, (VuoInteger)100);

				VuoInteger expectedCount = firstCountSeen + 1 + 98;
				QCOMPARE(countFromSummary, expectedCount);
				QCOMPARE(countFromRunner, expectedCount);

				incrementPortValue = 1000;
				runner->setInputPortValue(incrementPortIdentifier, VuoInteger_jsonFromValue(incrementPortValue));
			}
			else if (timesCountSentEvent == 3)
			{
				QCOMPARE(incrementFromRunner, (VuoInteger)1000);

				long expectedCount = firstCountSeen + 1 + 98 + 998;
				QCOMPARE(countFromSummary, expectedCount);
				QCOMPARE(countFromRunner, expectedCount);

				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
				isStopping = true;

				return;
			}
			++timesCountSentEvent;

			runner->unpause();
		}
	};

private slots:

	void testSettingAndGettingPortValues()
	{
		TestSettingAndGettingPortValuesRunnerDelegate delegate;
		delegate.runComposition();
	}

private:

	class TestGettingTriggerPortValuesRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		int timesCountChanged;
		VuoInteger firstCountSeen;
		string firedPortIdentifier;
		bool isStopping;

	public:
		TestGettingTriggerPortValuesRunnerDelegate()
		{
			runner = NULL;
			timesCountChanged = 0;
			isStopping = false;
		}

		~TestGettingTriggerPortValuesRunnerDelegate()
		{
			delete runner;
		}

		void runComposition()
		{
			string compositionPath = TestCompositionExecution::getCompositionPath("FirePeriodicallyWithCount.vuo");
			VuoCompilerBitcodeGenerator *generator = NULL;
			runner = createRunnerInNewProcess(compositionPath, &generator);

			foreach (VuoNode *node, generator->composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->getClassName() == "vuo.test.firePeriodicallyWithCount")
				{
					VuoPort *basePort = node->getOutputPortWithName("fired");
					firedPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
				}
			}
			delete generator;

			runner->setDelegate(this);

			runner->start();
			runner->waitUntilStopped();
		}

		void receivedTelemetryOutputPortUpdated(string portIdentifier, bool sentData, string dataSummary)
		{
			if (isStopping)
				return;

			runner->pause();

			QCOMPARE(QString(portIdentifier.c_str()), QString(firedPortIdentifier.c_str()));

			// For this value to be in sync with valueAsString, the composition needs to have been paused before the next event fired.
			// The firing rate of the composition is set slow enough that the composition is very likely to be paused in time.
			VuoInteger countFromRunner = VuoInteger_valueFromJson( runner->getOutputPortValue(firedPortIdentifier.c_str()) );

			VuoInteger countFromSummary = atol(dataSummary.c_str());

			if (timesCountChanged == 0)
			{
				// The composition may have started counting up before the runner was able to connect and start receiving telemetry.
				firstCountSeen = countFromSummary;
			}
			else if (timesCountChanged == 1)
			{
				VuoInteger expectedCount = firstCountSeen + 1;
				QCOMPARE(countFromSummary, expectedCount);
				QCOMPARE(countFromRunner, expectedCount);

				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
				isStopping = true;

				return;
			}
			++timesCountChanged;

			runner->unpause();
		}
	};

private slots:

	void testGettingTriggerPortValues()
	{
		TestGettingTriggerPortValuesRunnerDelegate delegate;
		delegate.runComposition();
	}

	void testGettingPublishedPorts_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< vector<VuoRunner::Port *> >("expectedInputs");
		QTest::addColumn< vector<VuoRunner::Port *> >("expectedOutputs");

		{
			vector<VuoRunner::Port *> inputs;
			inputs.push_back( new VuoRunner::Port("publishedIn0", new VuoType("VuoInteger")) );
			inputs.push_back( new VuoRunner::Port("publishedIn1", new VuoType("VuoInteger")) );
			vector<VuoRunner::Port *> outputs;
			outputs.push_back( new VuoRunner::Port("publishedSum", new VuoType("VuoInteger")) );
			QTest::newRow("some published input and output ports") << "Recur_Add_published.vuo" << inputs << outputs;
		}

		{
			vector<VuoRunner::Port *> inputs;
			vector<VuoRunner::Port *> outputs;
			QTest::newRow("no published ports") << "Recur_Count.vuo" << inputs << outputs;
		}
	}
	void testGettingPublishedPorts()
	{
		QFETCH(QString, compositionFile);
		QFETCH(vector<VuoRunner::Port *>, expectedInputs);
		QFETCH(vector<VuoRunner::Port *>, expectedOutputs);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoRunner *runner = createRunnerInNewProcess(compositionPath);
		runner->setDelegate(NULL);  /// @todo https://b33p.net/kosada/node/6021

		runner->startPaused();
		vector<VuoRunner::Port *> actualInputs = runner->getPublishedInputPorts();
		vector<VuoRunner::Port *> actualOutputs = runner->getPublishedOutputPorts();
		runner->stop();

		QCOMPARE(actualInputs.size(), expectedInputs.size());
		for (int i = 0; i < expectedInputs.size(); ++i)
		{
			QCOMPARE(actualInputs.at(i)->getName(), expectedInputs.at(i)->getName());
			QCOMPARE(actualInputs.at(i)->getType()->getModuleKey(), expectedInputs.at(i)->getType()->getModuleKey());
		}

		QCOMPARE(actualOutputs.size(), expectedOutputs.size());
		for (int i = 0; i < expectedOutputs.size(); ++i)
		{
			QCOMPARE(actualOutputs.at(i)->getName(), expectedOutputs.at(i)->getName());
			QCOMPARE(actualOutputs.at(i)->getType()->getModuleKey(), expectedOutputs.at(i)->getType()->getModuleKey());
		}

		delete runner;
	}

private:

	class TestPublishedPortCachingRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		bool isStopping;

	public:
		TestPublishedPortCachingRunnerDelegate(VuoRunner *runner)
		{
			this->runner = runner;
			isStopping = false;
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			// This tests that:
			//   - VuoRunner sets up published ports by the time VuoRunnerDelegate methods begin being called. - https://b33p.net/kosada/node/3880
			//   - The pointers retrieved by getPublishedOutputPorts are consistent.

			if (isStopping)
				return;

			vector<VuoRunner::Port *> outputPorts1 = runner->getPublishedOutputPorts();
			QCOMPARE(outputPorts1.size(), (size_t)1);
			QCOMPARE(outputPorts1.at(0), port);

			vector<VuoRunner::Port *> outputPorts2 = runner->getPublishedOutputPorts();
			QCOMPARE(outputPorts2.size(), (size_t)1);
			QCOMPARE(outputPorts2.at(0), port);

			dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
			dispatch_async(queue, ^{
							   runner->stop();
						   });
			isStopping = true;
		}
	};

private slots:

	void testPublishedPortCaching_data()
	{
		QTest::addColumn< bool >("shouldRunInSeparateProcess");

		QTest::newRow("separate process") << true;
		QTest::newRow("current process") << false;
	}
	void testPublishedPortCaching()
	{
		QFETCH(bool, shouldRunInSeparateProcess);

		string compositionPath = getCompositionPath("Recur_Add_published.vuo");

		VuoRunner *runner = (shouldRunInSeparateProcess ?
								 createRunnerInNewProcess(compositionPath) :
								 createRunnerInCurrentProcess(compositionPath));

		TestPublishedPortCachingRunnerDelegate delegate(runner);
		runner->setDelegate(&delegate);

		runner->start();

		if (shouldRunInSeparateProcess)
		{
			runner->waitUntilStopped();
		}
		else
		{
			dispatch_group_t group = dispatch_group_create();
			dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
			dispatch_group_async(group, queue, ^{
									 runner->waitUntilStopped();
								 });
			runner->runOnMainThread();
			dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
		}

		delete runner;
	}

private:

	class TestSettingAndGettingPublishedPortValuesRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		VuoRunner::Port *publishedIn1;
		VuoRunner::Port *publishedSum;
		int timesSumChanged;
		bool isStopping;

	public:
		TestSettingAndGettingPublishedPortValuesRunnerDelegate()
		{
			runner = NULL;
			timesSumChanged = 0;
			isStopping = false;
		}

		~TestSettingAndGettingPublishedPortValuesRunnerDelegate()
		{
			delete runner;
		}

		void runComposition()
		{
			string compositionPath = getCompositionPath("Recur_Add_published.vuo");
			runner = createRunnerInNewProcess(compositionPath);
			runner->setDelegate(this);

			runner->startPaused();

			VuoRunner::Port *publishedIn0 = runner->getPublishedInputPortWithName("publishedIn0");
			publishedIn1 = runner->getPublishedInputPortWithName("publishedIn1");
			publishedSum = runner->getPublishedOutputPortWithName("publishedSum");
			QVERIFY(publishedIn0 != NULL);
			QVERIFY(publishedIn1 != NULL);
			QVERIFY(publishedSum != NULL);

			runner->setPublishedInputPortValue(publishedIn0, VuoInteger_jsonFromValue(100));

			runner->unpause();
			runner->waitUntilStopped();
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			if (isStopping)
				return;

			runner->pause();

			QCOMPARE(QString(port->getName().c_str()), QString("publishedSum"));

			VuoInteger sumFromSummary = atol(dataSummary.c_str());

			++timesSumChanged;
			if (timesSumChanged == 1)
			{
				QCOMPARE(sumFromSummary, (VuoInteger)201);
				VuoInteger sumFromRunner = VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedSum) );
				QCOMPARE(sumFromRunner, (VuoInteger)201);

				runner->setPublishedInputPortValue(publishedIn1, VuoInteger_jsonFromValue(1000));
			}
			else
			{
				if (sumFromSummary == 201)
				{
					// Ignore the few additional events that may have fired before the composition was paused the first time.
					QVERIFY(timesSumChanged < 10);
				}
				else
				{
					QCOMPARE(sumFromSummary, (VuoInteger)1201);
					VuoInteger sumFromRunner = VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedSum) );
					QCOMPARE(sumFromRunner, (VuoInteger)1201);

					dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
					dispatch_async(queue, ^{
									   runner->stop();
								   });
					isStopping = true;

					return;
				}
			}

			runner->unpause();
		}
	};

private slots:

	void testSettingAndGettingPublishedPortValues()
	{
		TestSettingAndGettingPublishedPortValuesRunnerDelegate delegate;
		delegate.runComposition();
	}

private:

	class TestGeneratingEventOnPublishedInputPortRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		VuoRunner::Port *publishedDecrementBoth;
		int timesSumChanged;

	public:
		TestGeneratingEventOnPublishedInputPortRunnerDelegate()
		{
			runner = NULL;
			publishedDecrementBoth = NULL;
			timesSumChanged = 0;
		}

		~TestGeneratingEventOnPublishedInputPortRunnerDelegate()
		{
			delete runner;
		}

		void runComposition()
		{
			string compositionPath = getCompositionPath("PublishedInputsAndNoTrigger.vuo");
			runner = createRunnerInNewProcess(compositionPath);
			runner->setDelegate(this);

			runner->start();

			VuoRunner::Port *publishedIncrementOne = runner->getPublishedInputPortWithName("publishedIncrementOne");
			publishedDecrementBoth = runner->getPublishedInputPortWithName("publishedDecrementBoth");
			QVERIFY(publishedIncrementOne != NULL);
			QVERIFY(publishedDecrementBoth != NULL);

			runner->firePublishedInputPortEvent(publishedIncrementOne);

			runner->waitUntilStopped();
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			QCOMPARE(QString(port->getName().c_str()), QString("publishedSum"));

			++timesSumChanged;
			if (timesSumChanged == 1)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("42"));

				runner->setPublishedInputPortValue(publishedDecrementBoth, VuoInteger_jsonFromValue(3));
				runner->firePublishedInputPortEvent(publishedDecrementBoth);
			}
			else if (timesSumChanged == 2)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("36"));

				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
			}
		}
	};

private slots:

	void testGeneratingEventOnPublishedInputPort()
	{
		TestGeneratingEventOnPublishedInputPortRunnerDelegate delegate;
		delegate.runComposition();
	}

	void testWaitingForAnyPublishedPortEvent()
	{
		string compositionPath = getCompositionPath("MultiplePublishedOutputs.vuo");

		// Do twice: Generate an event and start to wait before it reaches any published output.
		{
			printf("    Trigger, wait, propagate\n");

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();

			VuoRunner::Port *publishedIncrement1 = runner->getPublishedInputPortWithName("publishedIncrement1");
			VuoRunner::Port *publishedCount1 = runner->getPublishedOutputPortWithName("publishedCount1");

			vector<VuoInteger> expectedCounts;
			expectedCounts.push_back(10);
			expectedCounts.push_back(20);
			for (vector<VuoInteger>::iterator i = expectedCounts.begin(); i != expectedCounts.end(); ++i)
			{
				runner->firePublishedInputPortEvent(publishedIncrement1);
				runner->waitForAnyPublishedOutputPortEvent();
				VuoInteger publishedCount1Value = VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedCount1) );
				QCOMPARE(publishedCount1Value, *i);
			}

			runner->stop();
			delete runner;
		}

		// Do twice: Generate an event and let it reach a published output port before starting to wait.
		{
			printf("    Trigger, propagate, wait\n");

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();

			VuoRunner::Port *publishedIncrement2 = runner->getPublishedInputPortWithName("publishedIncrement2");
			VuoRunner::Port *publishedCount2 = runner->getPublishedOutputPortWithName("publishedCount2");
			VuoRunner::Port *publishedCount3 = runner->getPublishedOutputPortWithName("publishedCount3");

			vector<VuoInteger> expectedCounts;
			expectedCounts.push_back(100);
			expectedCounts.push_back(200);
			for (vector<VuoInteger>::iterator i = expectedCounts.begin(); i != expectedCounts.end(); ++i)
			{
				runner->firePublishedInputPortEvent(publishedIncrement2);
				sleep(1);
				runner->waitForAnyPublishedOutputPortEvent();
				QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedCount2) ), *i);
				QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedCount3) ), *i);
			}

			runner->stop();
			delete runner;
		}

		// Do one of each. (Generate, wait, receive published output event. Generate, receive published output event, wait.)
		{
			printf("    Do one of each\n");

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();

			VuoRunner::Port *publishedIncrement1 = runner->getPublishedInputPortWithName("publishedIncrement1");
			VuoRunner::Port *publishedIncrement2 = runner->getPublishedInputPortWithName("publishedIncrement2");
			VuoRunner::Port *publishedCount1 = runner->getPublishedOutputPortWithName("publishedCount1");
			VuoRunner::Port *publishedCount2 = runner->getPublishedOutputPortWithName("publishedCount2");
			VuoRunner::Port *publishedCount3 = runner->getPublishedOutputPortWithName("publishedCount3");

			runner->firePublishedInputPortEvent(publishedIncrement1);
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedCount1) ), (VuoInteger)10);

			runner->firePublishedInputPortEvent(publishedIncrement2);
			sleep(1);
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedCount2) ), (VuoInteger)100);
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedCount3) ), (VuoInteger)100);

			runner->stop();
			delete runner;
		}

		// Generate more events than are waited on.
		{
			printf("    Trigger more than waited on\n");

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();

			VuoRunner::Port *publishedIncrement2 = runner->getPublishedInputPortWithName("publishedIncrement2");

			runner->firePublishedInputPortEvent(publishedIncrement2);
			runner->firePublishedInputPortEvent(publishedIncrement2);
			runner->waitForAnyPublishedOutputPortEvent();

			runner->stop();
			delete runner;
		}
	}

	void testReplacingCompositionWithoutCrashing_data()
	{
		QTest::addColumn< QString >("compositionFile");

		QTest::newRow("published trigger ports") << "PublishedInputsAndNoTrigger.vuo";
		QTest::newRow("non-repeating trigger port") << "Start.vuo";
		QTest::newRow("automatic repeating trigger port") << "Recur_Count.vuo";
		QTest::newRow("console window") << "Console.vuo";
	}
	void testReplacingCompositionWithoutCrashing()
	{
		QFETCH(QString, compositionFile);

		string compositionPath = getCompositionPath(compositionFile.toUtf8().constData());

		VuoCompiler *compiler = initCompiler();

		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		vector<string> alreadyLinkedResourcePaths;
		set<string> alreadyLinkedResources;

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;

		// Build and run the composition.
		{
			VuoCompilerGraphvizParser *parser = new VuoCompilerGraphvizParser(compositionPath, compiler);
			VuoComposition *baseComposition = new VuoComposition();
			composition = new VuoCompilerComposition(baseComposition, parser);
			delete parser;

			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource0", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath, resourceDylibPath);
			// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
			runner->start();
		}

		// Replace the composition with itself.
		{
			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource1", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionGraphviz = composition->getGraphvizDeclaration();
			string compositionDiff = composition->diffAgainstOlderComposition(compositionGraphviz, compiler);
			runner->replaceComposition(dylibPath, "", compositionDiff);
		}

		runner->stop();

		for (vector<string>::iterator i = alreadyLinkedResourcePaths.begin(); i != alreadyLinkedResourcePaths.end(); ++i)
			remove((*i).c_str());

		delete runner;
		delete compiler;
		delete composition;
	}

	void testAddingResourcesToRunningComposition()
	{
		string compositionPath = getCompositionPath("PublishedInputsAndNoTrigger.vuo");

		VuoCompiler *compiler = initCompiler();

		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		vector<string> alreadyLinkedResourcePaths;
		set<string> alreadyLinkedResources;

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;

		// Build and run an empty composition.
		{
			composition = new VuoCompilerComposition(new VuoComposition(), NULL);

			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource0", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath, resourceDylibPath, true);
			// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
			runner->start();
		}

		// Replace the composition with one in which a new node class has been added.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add "FireOnStart1".
			// (Unlike a stateless, non-event-firing node class, this will appear in the
			// compiled composition even without any incoming cables.)
			VuoCompilerNodeClass *fireOnStartNodeClass = compiler->getNodeClass("vuo.event.fireOnStart");
			VuoNode *fireOnStartNode = fireOnStartNodeClass->newNode("FireOnStart1");
			composition->getBase()->addNode(fireOnStartNode);

			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource1", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionDiff = composition->diffAgainstOlderComposition(oldCompositionGraphviz, compiler);
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);
		}

		// Replace the composition with one in which a new node class with library dependencies has been added.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add "DisplayConsoleWindow1".
			VuoCompilerNodeClass *displayConsoleWindowNodeClass = compiler->getNodeClass("vuo.console.window");
			VuoNode *displayConsoleWindowNode = displayConsoleWindowNodeClass->newNode("DisplayConsoleWindow1");
			composition->getBase()->addNode(displayConsoleWindowNode);

			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource2", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionDiff = composition->diffAgainstOlderComposition(oldCompositionGraphviz, compiler);
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);
		}

		runner->stop();

		for (vector<string>::iterator i = alreadyLinkedResourcePaths.begin(); i != alreadyLinkedResourcePaths.end(); ++i)
			remove((*i).c_str());

		delete runner;
		delete compiler;
		delete composition;
	}

	void testSerializingAndUnserializingComposition()
	{
		string compositionPath = getCompositionPath("PublishedInputsAndNoTrigger.vuo");

		VuoCompiler *compiler = initCompiler();

		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		vector<string> alreadyLinkedResourcePaths;
		set<string> alreadyLinkedResources;

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;
		VuoRunner::Port *publishedIncrementOne = NULL;
		VuoRunner::Port *publishedSum = NULL;
		VuoNode *count1Node = NULL;
		VuoNode *count2Node = NULL;
		VuoNode *makeList1Node = NULL;
		string originalCompositionGraphviz;

		// Build and run the original composition.
		{
			VuoCompilerGraphvizParser *parser = new VuoCompilerGraphvizParser(compositionPath, compiler);
			VuoComposition *baseComposition = new VuoComposition();
			composition = new VuoCompilerComposition(baseComposition, parser);
			delete parser;

			originalCompositionGraphviz = composition->getGraphvizDeclaration();

			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource0", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath, resourceDylibPath, true);
			// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
			runner->start();

			publishedIncrementOne = runner->getPublishedInputPortWithName("publishedIncrementOne");
			QVERIFY(publishedIncrementOne != NULL);
			publishedSum = runner->getPublishedOutputPortWithName("publishedSum");
			QVERIFY(publishedSum != NULL);

			// "Count1:increment" becomes 10, "Count1:count" becomes 10, "Add1:sum" becomes 10 + 0 = 10.
			runner->setPublishedInputPortValue(publishedIncrementOne, VuoInteger_jsonFromValue(10));
			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)10);
		}

		// Replace the composition with an identical one.
		{
			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource1", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionGraphviz = composition->getGraphvizDeclaration();
			string compositionDiff = composition->diffAgainstOlderComposition(compositionGraphviz, compiler);
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);

			// "Count1:increment" becomes 100, "Count1:count" becomes 110, "Add1:sum" becomes 110 + 0 = 110.
			runner->setPublishedInputPortValue(publishedIncrementOne, VuoInteger_jsonFromValue(100));
			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)110);
		}

		// Replace the composition with one in which a cable's endpoint has been changed.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			foreach (VuoNode *n, composition->getBase()->getNodes())
			{
				if (n->getTitle() == "Count1")
					count1Node = n;
				else if (n->getTitle() == "Count2")
					count2Node = n;
				else if (n->getTitle() == "MakeList1")
					makeList1Node = n;
			}
			QVERIFY(count1Node != NULL);
			QVERIFY(count2Node != NULL);
			QVERIFY(makeList1Node != NULL);

			VuoCable *cableToModify = NULL;
			foreach (VuoCable *c, composition->getBase()->getCables())
			{
				if (c->getFromNode() == count2Node && c->getToNode() == makeList1Node)
					cableToModify = c;
			}
			QVERIFY(cableToModify != NULL);

			// Change "Count2:count -> Add1:in1" to "Count1:count -> Add1:in1".
			VuoPort *count1NodeCountPort = count1Node->getOutputPortWithName("count");
			QVERIFY(count1NodeCountPort != NULL);
			cableToModify->setFrom(count1Node, count1NodeCountPort);

			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource2", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionDiff = composition->diffAgainstOlderComposition(oldCompositionGraphviz, compiler);
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);

			// "Count1:increment" becomes 1000, "Count1:count" becomes 1110, "Add1:sum" becomes 1110 + 1110 = 2220.
			runner->setPublishedInputPortValue(publishedIncrementOne, VuoInteger_jsonFromValue(1000));
			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)2220);
		}

		// Replace the composition with one in which a stateful node has been added.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add "Count3".
			VuoCompilerNodeClass *countNodeClass = count1Node->getNodeClass()->getCompiler();
			VuoNode *count3Node = countNodeClass->newNode("Count3");
			composition->getBase()->addNode(count3Node);

			// Remove "Count1:count -> MakeList1:item2".
			VuoCompilerPort *makeList1NodeItem2Port = static_cast<VuoCompilerPort *>(makeList1Node->getInputPortWithName("item2")->getCompiler());
			VuoCable *count1ToMakeList1Cable = NULL;
			foreach (VuoCable *c, composition->getBase()->getCables())
			{
				if (c->getFromNode() == count1Node && c->getToNode() == makeList1Node && c->getToPort() == makeList1NodeItem2Port->getBase())
					count1ToMakeList1Cable = c;
			}
			QVERIFY(count1ToMakeList1Cable != NULL);
			composition->getBase()->removeCable(count1ToMakeList1Cable);

			// Add "Count1:count -> Count3:increment".
			VuoCompilerPort *count1NodeCountPort = static_cast<VuoCompilerPort *>(count1Node->getOutputPortWithName("count")->getCompiler());
			VuoCompilerPort *count3NodeIncrementPort = static_cast<VuoCompilerPort *>(count3Node->getInputPortWithName("increment")->getCompiler());
			VuoCompilerCable *count1ToCount3Cable = new VuoCompilerCable(count1Node->getCompiler(), count1NodeCountPort,
																		 count3Node->getCompiler(), count3NodeIncrementPort);
			composition->getBase()->addCable(count1ToCount3Cable->getBase());

			// Add "Count3:count -> MakeList1:item2".
			VuoCompilerPort *count3NodeCountPort = static_cast<VuoCompilerPort *>(count3Node->getOutputPortWithName("count")->getCompiler());
			VuoCompilerCable *count3ToAdd1Cable = new VuoCompilerCable(count3Node->getCompiler(), count3NodeCountPort,
																	   makeList1Node->getCompiler(), makeList1NodeItem2Port);
			composition->getBase()->addCable(count3ToAdd1Cable->getBase());

			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource3", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionDiff = composition->diffAgainstOlderComposition(oldCompositionGraphviz, compiler);
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);

			// "Count1:increment" becomes 10000, "Count1:count" becomes 11110,
			// "Count3:increment" becomes 11110, "Count3:count" becomes 11110,
			// "Add1:sum" becomes 11110 + 11110 = 22220.
			runner->setPublishedInputPortValue(publishedIncrementOne, VuoInteger_jsonFromValue(10000));
			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)22220);

			// "Count1:increment" becomes 100000, "Count1:count" becomes 111110,
			// "Count3:increment" becomes 111110, "Count3:count" becomes 122220,
			// "Add1:sum" becomes 111110 + 122220 = 233330.
			runner->setPublishedInputPortValue(publishedIncrementOne, VuoInteger_jsonFromValue(100000));
			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)233330);
		}

		// Replace the latest composition with the original composition.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			VuoCompilerComposition *originalComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(originalCompositionGraphviz, compiler);

			VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(originalComposition, compiler);
			compiler->compileComposition(generator, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource4", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionDiff = originalComposition->diffAgainstOlderComposition(oldCompositionGraphviz, compiler);
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);
			delete originalComposition;

			// "Count1:increment" becomes 1000000, "Count1:count" becomes 1111110, "Add1:sum" becomes 1111110 + 122220 = 1233330.
			runner->setPublishedInputPortValue(publishedIncrementOne, VuoInteger_jsonFromValue(1000000));
			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(VuoInteger_valueFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)1233330);
		}

		runner->stop();

		for (vector<string>::iterator i = alreadyLinkedResourcePaths.begin(); i != alreadyLinkedResourcePaths.end(); ++i)
			remove((*i).c_str());

		delete runner;
		delete compiler;
		delete composition;
	}

};

QTEST_APPLESS_MAIN(TestControlAndTelemetry)
#include "TestControlAndTelemetry.moc"
